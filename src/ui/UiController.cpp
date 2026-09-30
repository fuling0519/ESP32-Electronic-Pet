#include "ui/UiController.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "hardware/Display.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
#include "storage/Memorials.h"
#include "ui/PetIcons.h"

namespace Ui {
namespace {
constexpr uint32_t kBootDurationMs = 1500;
constexpr uint32_t kGrowthTransitionDurationMs = 2400;
constexpr uint32_t kGrowthTransitionFrameMs = 200;
constexpr uint32_t kDeathAnimationDurationMs = 3600;
constexpr uint32_t kDeathAnimationFrameMs = 100;
constexpr uint32_t kBirdIdleCycleMs = 1100;
constexpr uint32_t kBirdIdleSecondFrameAtMs = 700;
constexpr uint32_t kSleepBirdFrameMs = 900;
constexpr uint32_t kFeedingDurationMs = 3000;
constexpr uint32_t kFeedingFrameMs = 250;
constexpr uint32_t kSleepZFrameMs = 650;
constexpr uint8_t kMenuItemCount = 7;
constexpr bool kDebugUiEvents = true;
const char* const kMenuItems[kMenuItemCount] = {
    "Feed", "Clean", "Treat", "Play", "Rest", "Status", "Records"
};
const char* const kMenuLabels[kMenuItemCount] = {
    "餵食", "清潔", "治療", "陪玩", "休息", "狀態", "紀錄"
};
constexpr int16_t kStatusLabelX = 9;
constexpr int16_t kStatusValueRight = 118;

void drawStatusNumber(Hardware::Display& display, int16_t baseline,
                      unsigned value) {
    char text[4];  // uint8_t values fit in three decimal digits.
    snprintf(text, sizeof(text), "%u", value);
    display.drawText(kStatusValueRight - static_cast<int16_t>(strlen(text) * 6),
                     baseline, text);
}

void drawStatusTextValue(Hardware::Display& display, int16_t baseline,
                         const char* text) {
    display.drawStatusText(kStatusValueRight - display.statusTextWidth(text),
                           baseline, text);
}

void drawCenteredUiText(Hardware::Display& display, int16_t baseline,
                        const char* text) {
    display.drawUiText(64 - static_cast<int16_t>(display.uiTextWidth(text) / 2),
                       baseline, text);
}

void formatAge(uint64_t age, char (&text)[14]) {
    if (age >= 86400) {
        snprintf(text, sizeof(text), "AGE %llud",
                 static_cast<unsigned long long>(age / 86400));
    } else if (age >= 3600) {
        snprintf(text, sizeof(text), "AGE %lluh",
                 static_cast<unsigned long long>(age / 3600));
    } else if (age >= 60) {
        snprintf(text, sizeof(text), "AGE %llum",
                 static_cast<unsigned long long>(age / 60));
    } else {
        snprintf(text, sizeof(text), "AGE %llus",
                 static_cast<unsigned long long>(age));
    }
}

bool isDeathFlowScreen(ScreenId screen) {
    return screen == ScreenId::DeathAnimation ||
           screen == ScreenId::DeathMemorial ||
           screen == ScreenId::DeathOptions ||
           screen == ScreenId::Graveyard ||
           screen == ScreenId::DeleteMemorialConfirm ||
           screen == ScreenId::AdoptionBlocked;
}

const char* inputEventName(Hardware::InputEvent event) {
    switch (event) {
        case Hardware::InputEvent::Up: return "UP";
        case Hardware::InputEvent::Down: return "DOWN";
        case Hardware::InputEvent::Left: return "LEFT";
        case Hardware::InputEvent::Right: return "RIGHT";
        case Hardware::InputEvent::Press: return "PRESS";
        case Hardware::InputEvent::LongPress: return "LONG_PRESS";
        case Hardware::InputEvent::None: return "NONE";
    }
    return "UNKNOWN";
}
}  // namespace

UiController::UiController(Hardware::Display& display, Hardware::Sound& sound,
                           const Pet::PetData& pet,
                           const Storage::Memorials& memorials)
    : display_(display), sound_(sound), pet_(pet), memorials_(memorials) {}

void UiController::init(uint32_t now) {
    screen_ = ScreenId::Boot;
    homeFocus_ = HomeFocus::None;
    menuIndex_ = 0;
    statusPage_ = 0;
    deathOptionIndex_ = 0;
    memorialIndex_ = 0;
    deleteConfirmIndex_ = 0;
    bootStartedAt_ = now;
    growthTransitionStartedAt_ = 0;
    growthTransitionElapsedMs_ = 0;
    growthTransitionFrame_ = 0;
    deathStartedAt_ = 0;
    deathAnimationElapsedMs_ = 0;
    deathAnimationFrame_ = 0;
    homeAnimationFrame_ = 0;
    feeding_ = false;
    sleepStartedAtMs_ = 0;
    sleepAnimationFrame_ = 0;
    sleepZPhase_ = 0;
    sleepVisualStage_ = pet_.lifeStage();
    eggAgeAtInitMilliseconds_ = pet_.lifeStage() == Pet::LifeStage::Egg ?
        pet_.ageSeconds() * 1000ULL : 0;
    eggAgeInitAtMs_ = now;
    eggCrackStage_ = eggCrackStage(now);
    observedLifeStage_ = pet_.lifeStage();
    dirty_ = true;
    pendingAction_ = UiAction::None;
    lastRenderedPetRevision_ = pet_.displayRevision();
}

uint8_t UiController::eggCrackStage(uint32_t now) const {
    if (pet_.lifeStage() != Pet::LifeStage::Egg) return 0;
    const uint64_t ageMilliseconds = eggAgeAtInitMilliseconds_ +
        static_cast<uint32_t>(now - eggAgeInitAtMs_);
    if (ageMilliseconds >= Pet::PetData::kEggLargeCrackAgeMilliseconds) return 2;
    if (ageMilliseconds >= Pet::PetData::kEggSmallCrackAgeMilliseconds) return 1;
    return 0;
}

bool UiController::update(Hardware::InputEvent event, uint32_t now) {
    const ScreenId previousScreen = screen_;
    const HomeFocus previousHomeFocus = homeFocus_;
    const uint8_t previousMenuIndex = menuIndex_;
    const uint8_t previousStatusPage = statusPage_;
    const uint8_t previousDeathOptionIndex = deathOptionIndex_;
    const uint8_t previousMemorialIndex = memorialIndex_;
    const uint8_t previousDeleteConfirmIndex = deleteConfirmIndex_;
    const uint8_t currentEggCrackStage = eggCrackStage(now);
    if (currentEggCrackStage != eggCrackStage_) {
        eggCrackStage_ = currentEggCrackStage;
        dirty_ = true;
    }

    if (kDebugUiEvents && event != Hardware::InputEvent::None) {
        Serial.print("UI received: ");
        Serial.println(inputEventName(event));
        Serial.print("UI screen: ");
        Serial.println(screenName());
    }

    if (screen_ == ScreenId::Boot) {
        if (now - bootStartedAt_ >= kBootDurationMs) {
            if (pet_.isDead()) beginDeathAnimation(now);
            else setScreen(ScreenId::Home);
        }
        return screen_ != previousScreen;
    }

    if (pet_.lifeStage() != observedLifeStage_ &&
        screen_ != ScreenId::Sleeping) {
        const Pet::LifeStage previousStage = observedLifeStage_;
        observedLifeStage_ = pet_.lifeStage();
        if (previousStage == Pet::LifeStage::Egg &&
            observedLifeStage_ == Pet::LifeStage::Baby) {
            beginGrowthTransition(ScreenId::HatchTransition, now);
            return true;
        }
        if (previousStage == Pet::LifeStage::Baby &&
            observedLifeStage_ == Pet::LifeStage::Adult) {
            beginGrowthTransition(ScreenId::GrowTransition, now);
            return true;
        }
    }

    if (pet_.isDead() && !isDeathFlowScreen(screen_)) {
        beginDeathAnimation(now);
        return true;
    }

    if (screen_ == ScreenId::DeathAnimation) {
        deathAnimationElapsedMs_ = now - deathStartedAt_;
        if (deathAnimationElapsedMs_ >= kDeathAnimationDurationMs) {
            setScreen(ScreenId::DeathMemorial);
        } else {
            const uint8_t frame = static_cast<uint8_t>(
                deathAnimationElapsedMs_ / kDeathAnimationFrameMs);
            if (frame != deathAnimationFrame_) {
                deathAnimationFrame_ = frame;
                dirty_ = true;
            }
        }
        return screen_ != previousScreen || dirty_;
    }

    if (screen_ == ScreenId::HatchTransition ||
        screen_ == ScreenId::GrowTransition) {
        growthTransitionElapsedMs_ = now - growthTransitionStartedAt_;
        if (growthTransitionElapsedMs_ >= kGrowthTransitionDurationMs) {
            setScreen(ScreenId::Home);
        } else {
            const uint8_t frame = static_cast<uint8_t>(
                growthTransitionElapsedMs_ / kGrowthTransitionFrameMs);
            if (frame != growthTransitionFrame_) {
                growthTransitionFrame_ = frame;
                dirty_ = true;
            }
        }
        return screen_ != previousScreen || dirty_;
    }

    switch (screen_) {
        case ScreenId::Home:
            if (feeding_) {
                const uint32_t elapsed = now - feedingStartedAt_;
                if (elapsed >= kFeedingDurationMs) {
                    feeding_ = false;
                    dirty_ = true;
                } else {
                    const uint8_t frame = (elapsed / kFeedingFrameMs) % 2;
                    const uint8_t bowlStage = elapsed / 1000;
                    if (frame != feedingFrame_ || bowlStage != feedingBowlStage_) {
                        feedingFrame_ = frame;
                        feedingBowlStage_ = bowlStage;
                        dirty_ = true;
                    }
                    // Finish the short animation before accepting navigation.
                    break;
                }
            }
            {
                const uint8_t frame = (now % kBirdIdleCycleMs) >=
                    kBirdIdleSecondFrameAtMs ? 1 : 0;
                if (frame != homeAnimationFrame_) {
                    homeAnimationFrame_ = frame;
                    dirty_ = true;
                }
            }
            if (event == Hardware::InputEvent::Right &&
                homeFocus_ == HomeFocus::None) {
                homeFocus_ = HomeFocus::Status;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Left &&
                       homeFocus_ == HomeFocus::Status) {
                homeFocus_ = HomeFocus::None;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press &&
                       homeFocus_ == HomeFocus::None) {
                sound_.playConfirm();
                setScreen(ScreenId::MainMenu);
            } else if (event == Hardware::InputEvent::Press &&
                       homeFocus_ == HomeFocus::Status) {
                sound_.playConfirm();
                setScreen(ScreenId::DetailedStatus);
            }
            break;

        case ScreenId::MainMenu:
            if (event == Hardware::InputEvent::Up) {
                menuIndex_ = (menuIndex_ + kMenuItemCount - 1) % kMenuItemCount;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Down) {
                menuIndex_ = (menuIndex_ + 1) % kMenuItemCount;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Right &&
                       menuIndex_ + 3 < kMenuItemCount) {
                menuIndex_ += 3;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Left && menuIndex_ >= 3) {
                menuIndex_ -= 3;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                sound_.playConfirm();
                switch (menuIndex_) {
                    case 0: setScreen(ScreenId::FeedCare); break;
                    case 1: setScreen(ScreenId::CleanCare); break;
                    case 2: setScreen(ScreenId::TreatCare); break;
                    case 3: setScreen(ScreenId::PlayCare); break;
                    case 4: setScreen(ScreenId::Rest); break;
                    case 5: setScreen(ScreenId::DetailedStatus); break;
                    default:
                        memorialIndex_ = 0;
                        setScreen(ScreenId::Graveyard);
                        break;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::Home);
            }
            break;

        case ScreenId::FeedCare:
        case ScreenId::CleanCare:
        case ScreenId::TreatCare:
        case ScreenId::PlayCare:
            if (event == Hardware::InputEvent::Press) {
                if (pet_.lifeStage() == Pet::LifeStage::Egg) {
                    sound_.playFailure();
                } else {
                    pendingAction_ = screen_ == ScreenId::FeedCare ? UiAction::Feed :
                        screen_ == ScreenId::CleanCare ? UiAction::Clean :
                        screen_ == ScreenId::TreatCare ? UiAction::Treat : UiAction::Play;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::MainMenu);
            }
            break;

        case ScreenId::Rest:
            if (event == Hardware::InputEvent::Press) {
                if (pet_.lifeStage() == Pet::LifeStage::Egg) {
                    sound_.playFailure();
                } else {
                    pendingAction_ = UiAction::StartNormalSleep;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::MainMenu);
            }
            break;

        case ScreenId::Sleeping:
            {
                const uint32_t elapsed = now - sleepStartedAtMs_;
                const uint8_t birdFrame = static_cast<uint8_t>(
                    (elapsed / kSleepBirdFrameMs) % 2);
                const uint8_t zPhase = static_cast<uint8_t>(
                    (elapsed / kSleepZFrameMs) % 3);
                if (birdFrame != sleepAnimationFrame_ ||
                    zPhase != sleepZPhase_) {
                    sleepAnimationFrame_ = birdFrame;
                    sleepZPhase_ = zPhase;
                    dirty_ = true;
                }
            }
            if (event == Hardware::InputEvent::Press) {
                pendingAction_ = UiAction::Wake;
            }
            break;

        case ScreenId::DetailedStatus:
            if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::Home);
            } else if ((event == Hardware::InputEvent::Right ||
                        event == Hardware::InputEvent::Down) && statusPage_ == 0) {
                statusPage_ = 1;
                dirty_ = true;
            } else if ((event == Hardware::InputEvent::Left ||
                        event == Hardware::InputEvent::Up) && statusPage_ == 1) {
                statusPage_ = 0;
                dirty_ = true;
            }
            break;

        case ScreenId::DeathMemorial:
            if (event == Hardware::InputEvent::Press) {
                sound_.playConfirm();
                deathOptionIndex_ = 0;
                setScreen(ScreenId::DeathOptions);
            }
            break;

        case ScreenId::DeathOptions:
            if (event == Hardware::InputEvent::Up ||
                event == Hardware::InputEvent::Down) {
                deathOptionIndex_ = deathOptionIndex_ == 0 ? 1 : 0;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                if (deathOptionIndex_ == 0) {
                    sound_.playConfirm();
                    memorialIndex_ = 0;
                    setScreen(ScreenId::Graveyard);
                } else if (!memorialReady_ ||
                           memorials_.count() > Storage::kMemorialLimit) {
                    sound_.playFailure();
                    setScreen(ScreenId::AdoptionBlocked);
                } else {
                    pendingAction_ = UiAction::AdoptNewEgg;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::DeathMemorial);
            }
            break;

        case ScreenId::Graveyard:
            if (memorials_.count() > 0 &&
                (event == Hardware::InputEvent::Up ||
                 event == Hardware::InputEvent::Left)) {
                memorialIndex_ = memorialIndex_ == 0 ?
                    memorials_.count() - 1 : memorialIndex_ - 1;
                dirty_ = true;
            } else if (memorials_.count() > 0 &&
                       (event == Hardware::InputEvent::Down ||
                        event == Hardware::InputEvent::Right)) {
                memorialIndex_ = (memorialIndex_ + 1) % memorials_.count();
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press &&
                       memorials_.count() > 0) {
                const Storage::MemorialRecord* selected = memorials_.at(memorialIndex_);
                if (pet_.isDead() && selected != nullptr &&
                    selected->petId == pet_.petId()) {
                    sound_.playFailure();
                } else {
                    sound_.playConfirm();
                    deleteConfirmIndex_ = 0;
                    setScreen(ScreenId::DeleteMemorialConfirm);
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(pet_.isDead() ? ScreenId::DeathOptions :
                          ScreenId::MainMenu);
            }
            break;

        case ScreenId::DeleteMemorialConfirm:
            if (event == Hardware::InputEvent::Left ||
                event == Hardware::InputEvent::Up) {
                deleteConfirmIndex_ = 0;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Right ||
                       event == Hardware::InputEvent::Down) {
                deleteConfirmIndex_ = 1;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                if (deleteConfirmIndex_ == 0) {
                    pendingAction_ = UiAction::DeleteMemorial;
                } else {
                    sound_.playCancel();
                    setScreen(ScreenId::Graveyard);
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::Graveyard);
            }
            break;

        case ScreenId::AdoptionBlocked:
            if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::DeathOptions);
            }
            break;

        case ScreenId::Boot:
        case ScreenId::HatchTransition:
        case ScreenId::GrowTransition:
        case ScreenId::DeathAnimation:
            break;
    }

    if (kDebugUiEvents && menuIndex_ != previousMenuIndex) {
        Serial.printf("Menu index: %u -> %u\n", previousMenuIndex, menuIndex_);
    }
    return screen_ != previousScreen || homeFocus_ != previousHomeFocus ||
           menuIndex_ != previousMenuIndex || statusPage_ != previousStatusPage ||
           deathOptionIndex_ != previousDeathOptionIndex ||
           memorialIndex_ != previousMemorialIndex ||
           deleteConfirmIndex_ != previousDeleteConfirmIndex;
}

void UiController::render() {
    if ((screen_ == ScreenId::Home || screen_ == ScreenId::Sleeping ||
         screen_ == ScreenId::DetailedStatus ||
         screen_ == ScreenId::FeedCare || screen_ == ScreenId::CleanCare ||
         screen_ == ScreenId::TreatCare || screen_ == ScreenId::PlayCare) &&
        pet_.displayRevision() != lastRenderedPetRevision_) dirty_ = true;
    if (!dirty_ || !display_.isInitialized()) return;

    switch (screen_) {
        case ScreenId::Boot: renderBoot(); break;
        case ScreenId::Home: renderHome(); break;
        case ScreenId::MainMenu: renderMainMenu(); break;
        case ScreenId::FeedCare: renderCare("餵食", "飽食", pet_.satiety()); break;
        case ScreenId::CleanCare: renderCare("清潔", "清潔", pet_.cleanliness()); break;
        case ScreenId::TreatCare:
            renderCare("治療", "健康", pet_.isSick() ? 1 : 0); break;
        case ScreenId::PlayCare: renderCare("陪玩", "心情", pet_.mood()); break;
        case ScreenId::Rest: renderRest(); break;
        case ScreenId::Sleeping: renderSleeping(); break;
        case ScreenId::DetailedStatus: renderDetailedStatus(); break;
        case ScreenId::HatchTransition: renderHatchTransition(); break;
        case ScreenId::GrowTransition: renderGrowTransition(); break;
        case ScreenId::DeathAnimation: renderDeathAnimation(); break;
        case ScreenId::DeathMemorial: renderDeathMemorial(); break;
        case ScreenId::DeathOptions: renderDeathOptions(); break;
        case ScreenId::Graveyard: renderGraveyard(); break;
        case ScreenId::DeleteMemorialConfirm: renderDeleteMemorialConfirm(); break;
        case ScreenId::AdoptionBlocked: renderAdoptionBlocked(); break;
    }
    display_.update();
    lastRenderedPetRevision_ = pet_.displayRevision();
    dirty_ = false;
}

ScreenId UiController::screen() const { return screen_; }

const char* UiController::screenName() const {
    switch (screen_) {
        case ScreenId::Boot: return "BOOT";
        case ScreenId::Home: return "HOME";
        case ScreenId::MainMenu: return "MAIN_MENU";
        case ScreenId::FeedCare: return "FEED_CARE";
        case ScreenId::CleanCare: return "CLEAN_CARE";
        case ScreenId::TreatCare: return "TREAT_CARE";
        case ScreenId::PlayCare: return "PLAY_CARE";
        case ScreenId::Rest: return "REST";
        case ScreenId::Sleeping: return "SLEEPING";
        case ScreenId::DetailedStatus: return "DETAILED_STATUS";
        case ScreenId::HatchTransition: return "HATCH_TRANSITION";
        case ScreenId::GrowTransition: return "GROW_TRANSITION";
        case ScreenId::DeathAnimation: return "DEATH_ANIMATION";
        case ScreenId::DeathMemorial: return "DEATH_MEMORIAL";
        case ScreenId::DeathOptions: return "DEATH_OPTIONS";
        case ScreenId::Graveyard: return "GRAVEYARD";
        case ScreenId::DeleteMemorialConfirm: return "DELETE_MEMORIAL_CONFIRM";
        case ScreenId::AdoptionBlocked: return "ADOPTION_BLOCKED";
    }
    return "UNKNOWN";
}

uint8_t UiController::menuIndex() const { return menuIndex_; }
const char* UiController::selectedMenuItem() const { return kMenuItems[menuIndex_]; }
UiAction UiController::takeAction() {
    const UiAction action = pendingAction_;
    pendingAction_ = UiAction::None;
    return action;
}

uint8_t UiController::selectedMemorialIndex() const { return memorialIndex_; }

void UiController::setMemorialReady(bool ready) {
    if (memorialReady_ == ready) return;
    memorialReady_ = ready;
    dirty_ = true;
}

void UiController::onFeedSucceeded(uint32_t now) {
    setScreen(ScreenId::Home);
    feeding_ = true;
    feedingStartedAt_ = now;
    feedingFrame_ = 0;
    feedingBowlStage_ = 0;
    dirty_ = true;
}

void UiController::onSleepStarted(uint32_t now) {
    sleepStartedAtMs_ = now;
    sleepAnimationFrame_ = 0;
    sleepZPhase_ = 0;
    sleepVisualStage_ = pet_.lifeStage();
    setScreen(ScreenId::Sleeping);
}

void UiController::onWakeSucceeded() {
    setScreen(ScreenId::Home);
}

void UiController::onAdoptionSucceeded(uint32_t now) {
    observedLifeStage_ = pet_.lifeStage();
    eggAgeAtInitMilliseconds_ = pet_.ageSeconds() * 1000ULL;
    eggAgeInitAtMs_ = now;
    eggCrackStage_ = eggCrackStage(now);
    pendingAction_ = UiAction::None;
    setScreen(ScreenId::Home);
}

void UiController::onMemorialDeleteResult(bool success) {
    if (!success) {
        sound_.playFailure();
        dirty_ = true;
        return;
    }
    sound_.playSuccess();
    if (memorialIndex_ >= memorials_.count() && memorialIndex_ > 0) {
        --memorialIndex_;
    }
    setScreen(ScreenId::Graveyard);
}

void UiController::beginGrowthTransition(ScreenId screen, uint32_t now) {
    pendingAction_ = UiAction::None;
    growthTransitionStartedAt_ = now;
    growthTransitionElapsedMs_ = 0;
    growthTransitionFrame_ = 0;
    if (screen == ScreenId::HatchTransition) sound_.playHatch();
    else sound_.playSuccess();
    setScreen(screen);
}

void UiController::beginDeathAnimation(uint32_t now) {
    pendingAction_ = UiAction::None;
    deathStartedAt_ = now;
    deathAnimationElapsedMs_ = 0;
    deathAnimationFrame_ = 0;
    sound_.playDeath();
    setScreen(ScreenId::DeathAnimation);
}

void UiController::setScreen(ScreenId screen) {
    if (screen_ == screen) return;
    screen_ = screen;
    feeding_ = false;
    if (screen_ == ScreenId::Home) homeFocus_ = HomeFocus::None;
    if (screen_ == ScreenId::DetailedStatus) statusPage_ = 0;
    dirty_ = true;
}

void UiController::renderBoot() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(20, 25, "ELECTRONIC PET");
    display_.drawText(37, 44, "Starting...");
}

void UiController::renderHome() {
    display_.clear();
    if (pet_.lifeStage() != Pet::LifeStage::Egg) {
        PetIcons::drawMood(display_, 3, 3, pet_.moodState());
        PetIcons::drawHunger(display_, 3, 22, pet_.hungerState());
    }

    if (feeding_) {
        PetIcons::drawEatingPet(display_, 32, 6, pet_.lifeStage(),
                               feedingFrame_, feedingBowlStage_);
    } else {
        PetIcons::drawPet(display_, 32, 6, pet_.lifeStage(),
                          eggCrackStage_, homeAnimationFrame_);
    }
    if (pet_.isSick()) PetIcons::drawSick(display_, 113, 7);
    if (pet_.lifeStage() != Pet::LifeStage::Egg) {
        PetIcons::drawCleaningAlert(display_, 113, 23, pet_.cleanlinessState());
    }
    PetIcons::drawStatusCard(display_, 113, 40);
    if (homeFocus_ == HomeFocus::Status) {
        // One-pixel breathing room keeps the selection frame distinct from
        // the card's own outline without changing the HUD layout.
        display_.drawFrame(111, 38, 16, 16);
    }

    renderPetFooter();
}

void UiController::renderPetFooter() {
    // Reserve y=55 as whitespace. This line is not yet tied to EXP progress.
    char levelText[6];  // "Lv255" plus terminator covers the uint8_t level.
    if (pet_.lifeStage() == Pet::LifeStage::Egg) strcpy(levelText, "EGG");
    else if (pet_.lifeStage() == Pet::LifeStage::Baby) strcpy(levelText, "BABY");
    else snprintf(levelText, sizeof(levelText), "Lv%u", static_cast<unsigned>(pet_.level()));
    const int16_t levelX = 128 - static_cast<int16_t>(strlen(levelText) * 5);
    display_.drawLine(3, 60, levelX - 6, 60);
    display_.drawSmallText(levelX, 63, levelText);
}

void UiController::renderMainMenu() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawUiText(9, 14, "選單");
    char pageText[4];
    snprintf(pageText, sizeof(pageText), "%u/3",
             static_cast<unsigned>(menuIndex_ / 3 + 1));
    display_.drawText(99, 14, pageText);
    display_.drawLine(6, 18, 121, 18);
    const uint8_t pageStart = (menuIndex_ / 3) * 3;
    for (uint8_t row = 0; row < 3; ++row) {
        const uint8_t i = pageStart + row;
        if (i >= kMenuItemCount) break;
        const int16_t baseline = 31 + (row * 15);
        if (i == menuIndex_) display_.drawSmallText(8, baseline, ">");
        display_.drawUiText(22, baseline, kMenuLabels[i]);
    }
}

void UiController::renderDetailedStatus() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(64 - static_cast<int16_t>(strlen(pet_.name()) * 3),
                      14, pet_.name());
    display_.drawText(99, 14, statusPage_ == 0 ? "1/2" : "2/2");
    display_.drawLine(6, 18, 121, 18);
    if (statusPage_ == 0) {
        renderDetailedStatusPage1();
    } else {
        renderDetailedStatusPage2();
    }
}

void UiController::renderDetailedStatusPage1() {
    display_.drawStatusText(kStatusLabelX, 30, "飽食");
    drawStatusNumber(display_, 30, static_cast<unsigned>(pet_.satiety()));

    display_.drawStatusText(kStatusLabelX, 45, "心情");
    drawStatusNumber(display_, 45, static_cast<unsigned>(pet_.mood()));

    display_.drawStatusText(kStatusLabelX, 60, "清潔");
    drawStatusNumber(display_, 60, static_cast<unsigned>(pet_.cleanliness()));
}

void UiController::renderDetailedStatusPage2() {
    display_.drawStatusText(kStatusLabelX, 30, "等級");
    drawStatusNumber(display_, 30, static_cast<unsigned>(pet_.level()));

    display_.drawStatusText(kStatusLabelX, 45, "健康");
    drawStatusTextValue(display_, 45, pet_.isSick() ? "生病" : "正常");

    display_.drawUiText(kStatusLabelX, 60, "階段");
    const char* stage = pet_.lifeStage() == Pet::LifeStage::Egg ? "蛋" :
        pet_.lifeStage() == Pet::LifeStage::Baby ? "幼鳥" : "成鳥";
    display_.drawUiText(kStatusValueRight - display_.uiTextWidth(stage), 60, stage);
}

void UiController::renderPlaceholder(const char* title) {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 15, title);
    display_.drawLine(6, 20, 121, 20);
    drawCenteredUiText(display_, 41, "尚未完成");
    drawCenteredUiText(display_, 59, "長按返回");
}

void UiController::renderCare(const char* title, const char* stat, unsigned value) {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 14, title);
    display_.drawLine(6, 19, 121, 19);
    if (pet_.lifeStage() == Pet::LifeStage::Egg) {
        drawCenteredUiText(display_, 38, "等待孵化");
        drawCenteredUiText(display_, 53, "無法使用");
        return;
    }

    display_.drawUiText(kStatusLabelX, 35, stat);
    if (screen_ == ScreenId::TreatCare) {
        drawStatusTextValue(display_, 35, pet_.isSick() ? "生病" : "正常");
    } else {
        drawStatusNumber(display_, 35, value);
    }
    const char* action = screen_ == ScreenId::FeedCare ? "按下執行 +20" :
        screen_ == ScreenId::CleanCare ? "按下執行 +30" :
        screen_ == ScreenId::PlayCare ? "按下執行 +15" : "按下執行";
    drawCenteredUiText(display_, 50, action);
    drawCenteredUiText(display_, 62, "長按返回");
}

void UiController::renderRest() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 15, "休息");
    display_.drawLine(6, 20, 121, 20);
    if (pet_.lifeStage() == Pet::LifeStage::Egg) {
        drawCenteredUiText(display_, 38, "等待孵化");
        drawCenteredUiText(display_, 53, "無法使用");
        return;
    }
    drawCenteredUiText(display_, 42, "按下執行");
    drawCenteredUiText(display_, 60, "長按返回");
}

void UiController::renderSleeping() {
    display_.clear();
    PetIcons::drawMood(display_, 3, 3, pet_.moodState());
    PetIcons::drawHunger(display_, 3, 22, pet_.hungerState());
    PetIcons::drawSleepingPet(display_, 32, 6, sleepVisualStage_,
                              sleepAnimationFrame_);
    if (pet_.isSick()) PetIcons::drawSick(display_, 113, 7);
    PetIcons::drawCleaningAlert(display_, 113, 23,
                                pet_.cleanlinessState());
    // Keep the familiar shortcut glyph, but omit the focus frame while asleep.
    PetIcons::drawStatusCard(display_, 113, 40);
    const char* zText = sleepZPhase_ == 0 ? "Z" :
        sleepZPhase_ == 1 ? "Zz" : "Zzz";
    display_.drawSmallText(83, 14, zText);
    renderPetFooter();
}

void UiController::renderHatchTransition() {
    display_.clear();
    if (growthTransitionElapsedMs_ < 700) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Egg, 2, 0);
    } else if (growthTransitionElapsedMs_ >= 1000) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Baby, 0,
                          growthTransitionFrame_ % 2);
        char message[20];
        snprintf(message, sizeof(message), "%s 孵化了！", pet_.name());
        drawCenteredUiText(display_, 62, message);
    } else {
        display_.drawLine(29, 22, 35, 22);
        display_.drawLine(32, 19, 32, 25);
        display_.drawLine(93, 14, 99, 14);
        display_.drawLine(96, 11, 96, 17);
    }
}

void UiController::renderGrowTransition() {
    display_.clear();
    if (growthTransitionElapsedMs_ < 700) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Baby, 0,
                          growthTransitionFrame_ % 2);
    } else if (growthTransitionElapsedMs_ >= 1000) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Adult, 0,
                          growthTransitionFrame_ % 2);
        drawCenteredUiText(display_, 62, "長大了!");
    } else {
        display_.drawLine(29, 22, 35, 22);
        display_.drawLine(32, 19, 32, 25);
        display_.drawLine(93, 14, 99, 14);
        display_.drawLine(96, 11, 96, 17);
    }
}

void UiController::renderDeathAnimation() {
    display_.clear();

    uint8_t dissolveStage = 0;
    if (deathAnimationElapsedMs_ >= 700) dissolveStage = 1;
    if (deathAnimationElapsedMs_ >= 1400) dissolveStage = 2;
    if (deathAnimationElapsedMs_ >= 2100) dissolveStage = 3;
    PetIcons::drawPetDissolve(display_, 32, 14, pet_.lifeStage(), dissolveStage);

    if (deathAnimationElapsedMs_ >= 300) {
        const uint32_t soulElapsed = deathAnimationElapsedMs_ - 300;
        const uint32_t capped = soulElapsed > 2900 ? 2900 : soulElapsed;
        const int16_t soulY = 23 - static_cast<int16_t>((capped * 22) / 2900);
        const uint8_t ghostFrame = static_cast<uint8_t>((soulElapsed / 300) % 2);
        PetIcons::drawGhost(display_, 64, soulY, ghostFrame);
    }

    if (deathAnimationElapsedMs_ >= 2200) {
        display_.drawLine(35, 12, 35, 16);
        display_.drawLine(33, 14, 37, 14);
        display_.drawLine(107, 21, 107, 23);
        display_.drawLine(106, 22, 108, 22);
    }
    display_.drawLine(26, 59, 101, 59);
}

void UiController::renderDeathMemorial() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    constexpr int16_t kTombstoneX = 10;
    constexpr int16_t kTombstoneWidth = 61;
    constexpr int16_t kTombstoneCenter =
        kTombstoneX + kTombstoneWidth / 2;
    PetIcons::drawTombstone(display_, kTombstoneX, 3,
                            kTombstoneWidth, 57);
    display_.drawText(
        kTombstoneCenter - static_cast<int16_t>(strlen(pet_.name()) * 3),
        22, pet_.name());
    display_.drawText(kTombstoneCenter - 9, 37, "RIP");

    char ageText[14];
    formatAge(pet_.ageSeconds(), ageText);
    const int16_t ageX = kTombstoneCenter -
        static_cast<int16_t>(strlen(ageText) * 5) / 2;
    display_.drawSmallText(ageX, 52, ageText);

    display_.drawUiText(90, 60, "> 繼續");
}

void UiController::renderDeathOptions() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 15, "接下來……？");
    display_.drawLine(6, 19, 121, 19);
    const char* options[] = {"探望墓園", "領養新蛋"};
    for (uint8_t i = 0; i < 2; ++i) {
        const int16_t baseline = 36 + i * 18;
        if (deathOptionIndex_ == i) display_.drawSmallText(9, baseline, ">");
        display_.drawUiText(24, baseline, options[i]);
    }
}

void UiController::renderGraveyard() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawUiText(8, 14, "墓碑群");
    char page[8];
    snprintf(page, sizeof(page), "%u/%u",
             static_cast<unsigned>(memorials_.count() == 0 ? 0 : memorialIndex_ + 1),
             static_cast<unsigned>(memorials_.count()));
    display_.drawText(94, 14, page);
    display_.drawLine(6, 18, 121, 18);
    const Storage::MemorialRecord* memorial = memorials_.at(memorialIndex_);
    if (memorial == nullptr) {
        drawCenteredUiText(display_, 40, "尚無紀錄");
        drawCenteredUiText(display_, 60, "長按返回");
        return;
    }
    PetIcons::drawTombstone(display_, 7, 23, 37, 37);
    display_.drawSmallText(18, 38, "RIP");
    display_.drawText(53, 34, memorial->name);
    char ageText[14];
    formatAge(memorial->ageSeconds, ageText);
    display_.drawSmallText(53, 46, ageText);
    if (!(pet_.isDead() && memorial->petId == pet_.petId())) {
        display_.drawUiText(87, 60, "> 刪除");
    }
}

void UiController::renderDeleteMemorialConfirm() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 16, "確定刪除?");
    display_.drawLine(6, 21, 121, 21);
    const Storage::MemorialRecord* memorial = memorials_.at(memorialIndex_);
    if (memorial != nullptr) {
        display_.drawText(64 - static_cast<int16_t>(strlen(memorial->name) * 3),
                          38, memorial->name);
    }
    constexpr int16_t kYesX = 32;
    constexpr int16_t kNoX = 80;
    display_.drawText(kYesX, 57, "Yes");
    display_.drawText(kNoX, 57, "No");
    display_.drawText((deleteConfirmIndex_ == 0 ? kYesX : kNoX) - 6,
                      57, ">");
}

void UiController::renderAdoptionBlocked() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    if (!memorialReady_) {
        display_.drawText(34, 28, "SAVE ERROR");
        drawCenteredUiText(display_, 46, "無法領養");
    } else {
        drawCenteredUiText(display_, 24, "紀錄已滿");
        drawCenteredUiText(display_, 43, "請先刪除");
    }
    drawCenteredUiText(display_, 61, "長按返回");
}

}  // namespace Ui
