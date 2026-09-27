#include "ui/UiController.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "hardware/Display.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
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
constexpr uint8_t kMenuItemCount = 6;
constexpr bool kDebugUiEvents = true;
const char* const kMenuItems[kMenuItemCount] = {
    "Feed", "Clean", "Treat", "Play", "Rest", "Status"
};
const char* const kMenuLabels[kMenuItemCount] = {
    "餵食", "清潔", "治療", "陪玩", "休息", "狀態"
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
                           const Pet::PetData& pet)
    : display_(display), sound_(sound), pet_(pet) {}

void UiController::init(uint32_t now) {
    screen_ = ScreenId::Boot;
    homeFocus_ = HomeFocus::None;
    menuIndex_ = 0;
    statusPage_ = 0;
    bootStartedAt_ = now;
    growthTransitionStartedAt_ = 0;
    growthTransitionElapsedMs_ = 0;
    growthTransitionFrame_ = 0;
    deathStartedAt_ = 0;
    deathAnimationElapsedMs_ = 0;
    deathAnimationFrame_ = 0;
    homeAnimationFrame_ = 0;
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

    if (pet_.lifeStage() != observedLifeStage_) {
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

    if (pet_.isDead() && screen_ != ScreenId::DeathAnimation &&
        screen_ != ScreenId::DeathMemorial) {
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

    if (screen_ == ScreenId::DeathMemorial) return false;

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
            } else if (event == Hardware::InputEvent::Right && menuIndex_ < 3) {
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
                    case 4: setScreen(ScreenId::RestPlaceholder); break;
                    default: setScreen(ScreenId::DetailedStatus); break;
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

        case ScreenId::RestPlaceholder:
            if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::MainMenu);
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

        case ScreenId::Boot:
        case ScreenId::HatchTransition:
        case ScreenId::GrowTransition:
        case ScreenId::DeathAnimation:
        case ScreenId::DeathMemorial:
            break;
    }

    if (kDebugUiEvents && menuIndex_ != previousMenuIndex) {
        Serial.printf("Menu index: %u -> %u\n", previousMenuIndex, menuIndex_);
    }
    return screen_ != previousScreen || homeFocus_ != previousHomeFocus ||
           menuIndex_ != previousMenuIndex || statusPage_ != previousStatusPage;
}

void UiController::render() {
    if ((screen_ == ScreenId::Home || screen_ == ScreenId::DetailedStatus ||
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
        case ScreenId::RestPlaceholder: renderPlaceholder("休息"); break;
        case ScreenId::DetailedStatus: renderDetailedStatus(); break;
        case ScreenId::HatchTransition: renderHatchTransition(); break;
        case ScreenId::GrowTransition: renderGrowTransition(); break;
        case ScreenId::DeathAnimation: renderDeathAnimation(); break;
        case ScreenId::DeathMemorial: renderDeathMemorial(); break;
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
        case ScreenId::RestPlaceholder: return "REST_PLACEHOLDER";
        case ScreenId::DetailedStatus: return "DETAILED_STATUS";
        case ScreenId::HatchTransition: return "HATCH_TRANSITION";
        case ScreenId::GrowTransition: return "GROW_TRANSITION";
        case ScreenId::DeathAnimation: return "DEATH_ANIMATION";
        case ScreenId::DeathMemorial: return "DEATH_MEMORIAL";
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

    PetIcons::drawPet(display_, 32, 6, pet_.lifeStage(),
                      eggCrackStage_, homeAnimationFrame_);
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

    // Reserve y=55 as whitespace. This baseline is not EXP progress.
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
    display_.drawText(99, 14, menuIndex_ < 3 ? "1/2" : "2/2");
    display_.drawLine(6, 18, 121, 18);
    const uint8_t pageStart = menuIndex_ < 3 ? 0 : 3;
    for (uint8_t row = 0; row < 3; ++row) {
        const uint8_t i = pageStart + row;
        const int16_t baseline = 31 + (row * 15);
        if (i == menuIndex_) display_.drawSmallText(8, baseline, ">");
        display_.drawUiText(22, baseline, kMenuLabels[i]);
    }
}

void UiController::renderDetailedStatus() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawStatusText(64 - display_.statusTextWidth("狀態") / 2, 14, "狀態");
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

void UiController::renderHatchTransition() {
    display_.clear();
    if (growthTransitionElapsedMs_ < 700) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Egg, 2, 0);
    } else if (growthTransitionElapsedMs_ >= 1000) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Baby, 0,
                          growthTransitionFrame_ % 2);
        drawCenteredUiText(display_, 62, "孵化了!");
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
    PetIcons::drawTombstone(display_, 43, 7);
    display_.drawText(55, 30, "RIP");

    char ageText[14];
    const uint64_t age = pet_.ageSeconds();
    if (age >= 86400) {
        snprintf(ageText, sizeof(ageText), "AGE %llud",
                 static_cast<unsigned long long>(age / 86400));
    } else if (age >= 3600) {
        snprintf(ageText, sizeof(ageText), "AGE %lluh",
                 static_cast<unsigned long long>(age / 3600));
    } else if (age >= 60) {
        snprintf(ageText, sizeof(ageText), "AGE %llum",
                 static_cast<unsigned long long>(age / 60));
    } else {
        snprintf(ageText, sizeof(ageText), "AGE %llus",
                 static_cast<unsigned long long>(age));
    }
    const int16_t ageX = 64 - static_cast<int16_t>(strlen(ageText) * 5) / 2;
    display_.drawSmallText(ageX, 43, ageText);

    display_.drawLine(34, 56, 94, 56);
    display_.drawLine(37, 56, 34, 52);
    display_.drawLine(48, 56, 46, 53);
    display_.drawLine(80, 56, 82, 52);
    display_.drawLine(91, 56, 94, 53);
}

}  // namespace Ui
