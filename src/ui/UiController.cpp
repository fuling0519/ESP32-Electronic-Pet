#include "ui/UiController.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>

#include "hardware/Display.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
#include "storage/Memorials.h"
#include "ui/PetIcons.h"
#include "ui/RpsIcons.h"
#include "ui/PotionSprite.h"
#include "ui/CleaningSprite.h"

namespace Ui {
namespace {
constexpr uint32_t kBootDurationMs = 1500;
#if defined(PET_SAD_TEST_MODE)
constexpr uint32_t kCareReminderCooldownMs = 10000;
#else
constexpr uint32_t kCareReminderCooldownMs = 300000;
#endif
constexpr uint32_t kFarewellPoseMs = 2600;
constexpr uint32_t kFarewellFlashMs = 100;
constexpr uint32_t kGrowthTransitionDurationMs = 2400;
constexpr uint32_t kGrowthTransitionFrameMs = 200;
constexpr uint32_t kDeathAnimationDurationMs = 3600;
constexpr uint32_t kDeathAnimationFrameMs = 100;
constexpr uint32_t kBirdIdleCycleMs = 2600;
constexpr uint32_t kBirdIdleSecondFrameAtMs = 1600;
constexpr uint32_t kSleepBirdFrameMs = 900;
constexpr uint32_t kFeedingDurationMs = 3000;
constexpr uint32_t kFeedingFrameMs = 250;
constexpr uint32_t kSleepZFrameMs = 650;
constexpr uint32_t kTreatmentPourDoneMs = 1200;
constexpr uint32_t kTreatmentSparkleStartMs = 1400;
constexpr uint32_t kTreatmentDurationMs = 3400;
constexpr uint32_t kTreatmentNoticeMs = 1200;
constexpr uint32_t kCleaningFrameMs = 400;
constexpr uint32_t kCleaningDurationMs = 2400;
constexpr uint8_t kMenuItemCount = 9;
constexpr bool kDebugUiEvents = true;
const char* const kMenuItems[kMenuItemCount] = {
    "Feed", "Clean", "Treat", "Play", "Rest", "Status", "Records", "Farewell", "Sound"
};
const char* const kMenuLabels[kMenuItemCount] = {
    "餵食", "清潔", "治療", "陪玩", "休息", "狀態", "紀念冊", "送別", "聲音"
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

void drawSparkles(Hardware::Display& display, uint8_t frame,
                  int16_t leftX, int16_t rightX) {
    // Each star fades to a point before reappearing at its other height.
    // Their different phases and travel distances keep the sparkle irregular.
    const uint8_t leftLight[]  = {0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0, 1, 2, 0};
    const uint8_t rightLight[] = {0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0};
    const uint8_t step = frame % 16;
    const auto star = [&](int16_t x, int16_t y, uint8_t light) {
        if (!light) return;
        display.drawLine(x, y, x, y);
        if (light >= 2) {
            display.drawLine(x - 1, y, x + 1, y);
            display.drawLine(x, y - 1, x, y + 1);
        }
        if (light >= 3) {
            display.drawLine(x - 2, y, x + 2, y);
            display.drawLine(x, y - 2, x, y + 2);
        }
    };
    star(leftX, step >= 7 && step <= 11 ? 39 : 23, leftLight[step]);
    star(rightX, step >= 9 ? 25 : 37, rightLight[step]);
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
           screen == ScreenId::MemorialCategories ||
           screen == ScreenId::MemorialHelp ||
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
                           const Storage::Memorials& memorials,
                           Games::RpsGame::RandomSource random)
    : display_(display), sound_(sound), pet_(pet), memorials_(memorials), game_(random), memoryNotes_(random) {}

void UiController::init(uint32_t now) {
    screen_ = ScreenId::Boot;
    homeFocus_ = HomeFocus::None;
    menuIndex_ = 0;
    memorialFilter_ = 0;
    memorialCategoryIndex_ = 0;
    farewellConfirmIndex_ = 0;
    treatmentNotice_ = false;
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
    cleaning_ = false;
    cleaningStartedAt_ = 0;
    cleaningFrame_ = 0;
    treating_ = treatmentActionQueued_ = treatmentSucceeded_ = false;
    treatmentElapsedMs_ = 0;
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
    normalSleepFailed_ = storageSaveFailed_ = storageLow_ = false;
    game_.cancel();
    memoryNotes_.cancel();
    selectedGame_ = 0;
    gameMoodGain_ = 0;
    gameExpGain_ = 0;
    gamePreviousLevel_ = gameFinalLevel_ = 0;
    levelUpActive_ = false;
    lastRenderedPetRevision_ = pet_.displayRevision();
    careLevelUpPending_ = false;
    levelUpPreviousLevel_ = levelUpFinalLevel_ = 0;
    sadMood_ = sadSatiety_ = sadCleanliness_ = sad_ = false;
    observedSick_ = observedSleeping_ = false;
    careReminderPlayed_ = false;
    observedPetId_ = pet_.petId();
    updateSadState();
    careReminderPending_ = false; // Loading a sad pet is silent.
}

void UiController::updateSadState() {
    if (observedPetId_ != pet_.petId()) {
        observedPetId_ = pet_.petId();
        sadMood_ = sadSatiety_ = sadCleanliness_ = sad_ = false;
        observedSick_ = observedSleeping_ = false;
        careReminderPending_ = careReminderPlayed_ = false;
    }
    const bool eligible = !pet_.isEnded() && pet_.lifeStage() != Pet::LifeStage::Egg;
    const bool sleeping = pet_.sleepMode() != Pet::SleepMode::Awake;
    const bool sick = eligible && pet_.isSick();
    sadMood_ = eligible && (sadMood_ ? pet_.mood() < 45 : pet_.mood() <= 40);
    sadSatiety_ = eligible && (sadSatiety_ ? pet_.satiety() < 30 : pet_.satiety() <= 25);
    sadCleanliness_ = eligible && (sadCleanliness_ ? pet_.cleanliness() < 30 : pet_.cleanliness() <= 25);
    const bool next = sick || sadMood_ || sadSatiety_ || sadCleanliness_;
    if ((!sad_ && next) || (!observedSick_ && sick) ||
        (observedSleeping_ && !sleeping && next)) careReminderPending_ = true;
    if (!next) careReminderPending_ = false;
    if (sad_ != next) dirty_ = true;
    sad_ = next;
    observedSick_ = sick;
    observedSleeping_ = sleeping;
}

uint8_t UiController::eggCrackStage(uint32_t now) const {
    if (pet_.lifeStage() != Pet::LifeStage::Egg) return 0;
    const uint64_t ageMilliseconds = eggAgeAtInitMilliseconds_ +
        static_cast<uint32_t>(now - eggAgeInitAtMs_);
    if (ageMilliseconds >= Pet::PetData::kEggLargeCrackAgeMilliseconds) return 2;
    if (ageMilliseconds >= Pet::PetData::kEggSmallCrackAgeMilliseconds) return 1;
    return 0;
}

bool UiController::update(Hardware::InputEvent event, uint32_t now, bool directionNeutral) {
    updateSadState();
    const ScreenId previousScreen = screen_;
    const HomeFocus previousHomeFocus = homeFocus_;
    const uint8_t previousMenuIndex = menuIndex_;
    const uint8_t previousStatusPage = statusPage_;
    const uint8_t previousDeathOptionIndex = deathOptionIndex_;
    const uint8_t previousMemorialIndex = memorialIndex_;
    const uint8_t previousMemorialCategoryIndex = memorialCategoryIndex_;
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
            if (pet_.isDeparted()) setScreen(memorialReady_ ? ScreenId::FarewellDone : ScreenId::FarewellBlocked);
            else if (pet_.isDead()) beginDeathAnimation(now);
            else setScreen(ScreenId::Home);
        }
        return screen_ != previousScreen;
    }

    if (screen_ == ScreenId::FarewellAnimation) {
        farewellElapsedMs_ = now - farewellStartedAt_;
        if (farewellElapsedMs_ >= kFarewellPoseMs + kFarewellFlashMs) setScreen(ScreenId::FarewellDone);
        dirty_ = true;
        return true; // Includes the completion event: no accidental adoption.
    }

    if (pet_.lifeStage() != observedLifeStage_ &&
        screen_ != ScreenId::Sleeping && !treating_ && !cleaning_) {
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

    if (pet_.isDead() && !isDeathFlowScreen(screen_) &&
        !(screen_ == ScreenId::Volume && volumeReturnScreen_ == ScreenId::DeathOptions)) {
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

    // Keep care input locked through pouring and recovery. Only emit Treat once,
    // after the liquid is gone; the application confirms the actual result.
    if (treating_) {
        if (pet_.sleepMode() != Pet::SleepMode::Awake) {
            treating_ = false;
            if (pendingAction_ == UiAction::Treat) pendingAction_ = UiAction::None;
            dirty_ = true;
        } else {
            treatmentElapsedMs_ = now - treatmentStartedAt_;
            if (!treatmentActionQueued_ && treatmentElapsedMs_ >= kTreatmentPourDoneMs) {
                treatmentActionQueued_ = true;
                pendingAction_ = UiAction::Treat;
            }
            if (treatmentSucceeded_ && treatmentElapsedMs_ >= kTreatmentDurationMs) {
                treating_ = false;
            }
            dirty_ = true;
            return true;
        }
    }

    // Keep the confirmed rising-water/full-white sequence on Home. Consume even
    // the completion event so a held/repeated press cannot reopen the menu.
    if (cleaning_) {
        const uint32_t elapsed = now - cleaningStartedAt_;
        if (elapsed >= kCleaningDurationMs ||
            pet_.sleepMode() != Pet::SleepMode::Awake) {
            cleaning_ = false;
            dirty_ = true;
        } else {
            const uint8_t frame = elapsed >= 3 * kCleaningFrameMs ? 3 :
                static_cast<uint8_t>(elapsed / kCleaningFrameMs);
            if (frame != cleaningFrame_) {
                cleaningFrame_ = frame;
                dirty_ = true;
            }
        }
        return dirty_;
    }

    // Care celebrations wait for feeding/cleaning/growth, and consume navigation while shown.
    if (screen_ != ScreenId::PlayCare &&
        (careLevelUpPending_ || levelUpActive_)) {
        if (pet_.isDead() || pet_.sleepMode() != Pet::SleepMode::Awake) {
            careLevelUpPending_ = levelUpActive_ = false;
        } else if (levelUpActive_) {
            if (event == Hardware::InputEvent::LongPress) {
                levelUpActive_ = false;
                cancelTo(ScreenId::MainMenu);
            } else {
                const uint32_t elapsed = now - levelUpStartedAt_;
                levelUpFrame_ = static_cast<uint8_t>(elapsed / 125);
                if (elapsed >= 2000) levelUpActive_ = false;
            }
            dirty_ = true;
            return true;
        } else if ((!feeding_ || now - feedingStartedAt_ >= kFeedingDurationMs) &&
                   now - careRewardAt_ >= 400) {
            feeding_ = false;
            careLevelUpPending_ = false;
            levelUpActive_ = true;
            levelUpStartedAt_ = now;
            levelUpFrame_ = 0;
            sound_.playLevelUp();
            dirty_ = true;
            return true;
        } else if (!feeding_) {
            return true;
        }
    }

    switch (screen_) {
        case ScreenId::Home:
            if (feeding_) {
                const uint32_t elapsed = now - feedingStartedAt_;
                if (elapsed >= kFeedingDurationMs) {
                    feeding_ = false;
                    dirty_ = true;
                } else {
                    const bool wyvern = pet_.speciesId() == Pet::SpeciesId::Wyvern;
                    const uint8_t frame = (elapsed / (wyvern ? 375 : kFeedingFrameMs)) % (wyvern ? 4 : 2);
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
                const bool wyvern = pet_.speciesId() == Pet::SpeciesId::Wyvern &&
                    pet_.lifeStage() != Pet::LifeStage::Egg;
                const uint8_t frame = wyvern ? (sad_ ? (now / 1000) % 2 :
                    ((now % 1000) * 6 / 1000) % 2) :
                    ((now % kBirdIdleCycleMs) >= kBirdIdleSecondFrameAtMs ? 1 : 0);
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
                confirmTo(ScreenId::MainMenu);
            } else if (event == Hardware::InputEvent::Press &&
                       homeFocus_ == HomeFocus::Status) {
                confirmTo(ScreenId::DetailedStatus);
            }
            break;

        case ScreenId::MainMenu:
            if (treatmentNotice_ && (event != Hardware::InputEvent::None ||
                now - treatmentNoticeStartedAt_ >= kTreatmentNoticeMs)) {
                treatmentNotice_ = false;
                dirty_ = true;
            }
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
                if (pet_.lifeStage() == Pet::LifeStage::Egg &&
                    (menuIndex_ <= 4 || menuIndex_ == 7)) {
                    sound_.playFailure();
                    break;
                }
                if (menuIndex_ != 8) sound_.playConfirm();
                switch (menuIndex_) {
                    case 0: setScreen(ScreenId::FeedCare); break;
                    case 1: setScreen(ScreenId::CleanCare); break;
                    case 2: beginTreatment(now); break;
                    case 3:
                        selectedGame_ = 0;
                        setScreen(ScreenId::GameSelect); break;
                    case 4:
                        restOption_ = 0; deepSleepFailed_ = normalSleepFailed_ = false;
                        setScreen(ScreenId::Rest); break;
                    case 5: setScreen(ScreenId::DetailedStatus); break;
                    case 8: beginVolume(ScreenId::MainMenu); break;
                    case 7:
                        if (pet_.lifeStage() == Pet::LifeStage::Egg) sound_.playFailure();
                        else if (memorials_.count() >= Storage::kMemorialLimit) setScreen(ScreenId::FarewellBlocked);
                        else setScreen(ScreenId::FarewellInfo);
                        break;
                    default:
                        memorialCategoryIndex_ = 0;
                        setScreen(ScreenId::MemorialCategories);
                        break;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::Home);
            }
            break;

        case ScreenId::FeedCare:
        case ScreenId::CleanCare:
            if (event == Hardware::InputEvent::Press) {
                if (pet_.lifeStage() == Pet::LifeStage::Egg) {
                    sound_.playFailure();
                } else {
                    pendingAction_ = screen_ == ScreenId::FeedCare ?
                        UiAction::Feed : UiAction::Clean;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MainMenu);
            }
            break;

        case ScreenId::GameSelect:
            if (event == Hardware::InputEvent::Up || event == Hardware::InputEvent::Down) {
                selectedGame_ = 1 - selectedGame_;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                game_.begin(); memoryNotes_.begin();
                gameMoodGain_ = 0; gameExpGain_ = 0;
                gamePreviousLevel_ = gameFinalLevel_ = 0;
                confirmTo(ScreenId::PlayCare);
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MainMenu);
            }
            break;
        case ScreenId::PlayCare:
            if (selectedGame_ == 1) updateMemoryNotes(event, now, directionNeutral);
            else updateGame(event, now);
            break;

        case ScreenId::Rest:
            if (deepSleepPending_) break;
            if (event == Hardware::InputEvent::Up || event == Hardware::InputEvent::Down) {
                restOption_ = 1 - restOption_; deepSleepFailed_ = normalSleepFailed_ = false; dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                if (pet_.lifeStage() == Pet::LifeStage::Egg) {
                    sound_.playFailure();
                } else {
                    pendingAction_ = restOption_ == 0 ? UiAction::StartNormalSleep : UiAction::StartDeepSleep;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MainMenu);
            }
            break;

        case ScreenId::Sleeping:
            {
                const uint32_t elapsed = now - sleepStartedAtMs_;
                const bool wyvern = pet_.speciesId() == Pet::SpeciesId::Wyvern;
                const uint8_t birdFrame = wyvern && sleepVisualStage_ == Pet::LifeStage::Adult ? 0 :
                    static_cast<uint8_t>((elapsed / (wyvern ? 1000 : kSleepBirdFrameMs)) % 2);
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

        case ScreenId::Volume:
            if (event == Hardware::InputEvent::Down ||
                (event == Hardware::InputEvent::Press && !volumeButtonsFocused_)) {
                if (!volumeButtonsFocused_) {
                    volumeButtonsFocused_ = true;
                    volumeCancelSelected_ = false;
                    dirty_ = true;
                }
            } else if (event == Hardware::InputEvent::Up) {
                volumeButtonsFocused_ = false;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Left || event == Hardware::InputEvent::Right) {
                if (volumeButtonsFocused_) {
                    volumeCancelSelected_ = !volumeCancelSelected_;
                    dirty_ = true;
                    break;
                }
                const uint8_t before = sound_.volume();
                const uint8_t next = before ? 0 : 1;
                if (next != before) {
                    sound_.stopTone();
                    sound_.setVolume(next);
                    if (next) sound_.playTone(1047, 120);
                    volumeSaveFailed_ = false;
                    dirty_ = true;
                }
            } else if (event == Hardware::InputEvent::Press) {
                if (volumeCancelSelected_) {
                    cancelTo(volumeReturnScreen_);
                } else if (sound_.volume() == originalVolume_) {
                    confirmTo(volumeReturnScreen_);
                }
                else pendingAction_ = UiAction::SaveVolume;
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(volumeReturnScreen_);
            }
            break;

        case ScreenId::DetailedStatus:
            if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::Home);
            } else if ((event == Hardware::InputEvent::Right ||
                        event == Hardware::InputEvent::Down) && statusPage_ < 3) {
                ++statusPage_;
                dirty_ = true;
            } else if ((event == Hardware::InputEvent::Left ||
                        event == Hardware::InputEvent::Up) && statusPage_ > 0) {
                --statusPage_;
                dirty_ = true;
            }
            break;

        case ScreenId::FarewellDone:
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
                deathOptionIndex_ = (deathOptionIndex_ +
                    (event == Hardware::InputEvent::Down ? 1 : 2)) % 3;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                if (deathOptionIndex_ == 0) {
                    sound_.playConfirm();
                    memorialCategoryIndex_ = 0;
                    setScreen(ScreenId::MemorialCategories);
                } else if (deathOptionIndex_ == 2) {
                    beginVolume(ScreenId::DeathOptions);
                } else if (!memorialReady_ ||
                           memorials_.count() > Storage::kMemorialLimit) {
                    sound_.playFailure();
                    setScreen(ScreenId::AdoptionBlocked);
                } else {
                    pendingAction_ = UiAction::AdoptNewEgg;
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(pet_.isDeparted() ? ScreenId::FarewellDone : ScreenId::DeathMemorial);
            }
            break;

        case ScreenId::MemorialCategories:
            if (event == Hardware::InputEvent::Up || event == Hardware::InputEvent::Down) {
                memorialCategoryIndex_ = (memorialCategoryIndex_ +
                    (event == Hardware::InputEvent::Down ? 1 : 2)) % 3;
                dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                sound_.playConfirm();
                if (memorialCategoryIndex_ == 2) setScreen(ScreenId::MemorialHelp);
                else {
                    memorialFilter_ = memorialCategoryIndex_ + 1;
                    memorialIndex_ = 255;
                    selectMemorial(1);
                    setScreen(ScreenId::Graveyard);
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(pet_.isEnded() ? ScreenId::DeathOptions : ScreenId::MainMenu);
            }
            break;
        case ScreenId::MemorialHelp:
            if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MemorialCategories);
            }
            break;
        case ScreenId::Graveyard:
            if (event == Hardware::InputEvent::Left || event == Hardware::InputEvent::Right) {
                selectMemorial(event == Hardware::InputEvent::Right ? 1 : -1);
            } else if (event == Hardware::InputEvent::Press && matchesMemorial(memorialIndex_)) {
                const auto* selected = memorials_.at(memorialIndex_);
                if (pet_.isEnded() && selected->petId == pet_.petId()) sound_.playFailure();
                else {
                    deleteConfirmIndex_ = 1; // Default to keeping the memory.
                    confirmTo(ScreenId::DeleteMemorialConfirm);
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MemorialCategories);
            }
            break;

        case ScreenId::FarewellInfo:
            if (event == Hardware::InputEvent::Press) {
                farewellConfirmIndex_ = 0;
                confirmTo(ScreenId::FarewellConfirm);
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MainMenu);
            }
            break;
        case ScreenId::FarewellConfirm:
            if (event == Hardware::InputEvent::Left || event == Hardware::InputEvent::Up) {
                farewellConfirmIndex_ = 0; dirty_ = true;
            } else if (event == Hardware::InputEvent::Right || event == Hardware::InputEvent::Down) {
                farewellConfirmIndex_ = 1; dirty_ = true;
            } else if (event == Hardware::InputEvent::Press) {
                if (farewellConfirmIndex_ == 0) {
                    cancelTo(ScreenId::MainMenu);
                }
                else {
                    pendingAction_ = UiAction::SendOff;
                    setScreen(ScreenId::FarewellBlocked); // Lock duplicate input until result.
                    sound_.playConfirm();
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::MainMenu);
            }
            break;
        case ScreenId::FarewellBlocked:
            if (pet_.isDeparted() && memorialReady_) setScreen(ScreenId::FarewellDone);
            else if (event == Hardware::InputEvent::Press) {
                sound_.playConfirm();
                if (pet_.isDeparted()) pendingAction_ = UiAction::RetryFarewell;
                else if (memorials_.count() >= Storage::kMemorialLimit) {
                    memorialCategoryIndex_ = 0;
                    setScreen(ScreenId::MemorialCategories);
                } else {
                    farewellConfirmIndex_ = 0;
                    setScreen(ScreenId::FarewellConfirm);
                }
            } else if (event == Hardware::InputEvent::LongPress && !pet_.isDeparted()) {
                cancelTo(ScreenId::MainMenu);
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
                    cancelTo(ScreenId::Graveyard);
                }
            } else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::Graveyard);
            }
            break;

        case ScreenId::AdoptionBlocked:
            if (event == Hardware::InputEvent::Press && !memorialReady_) {
                sound_.playConfirm();
                pendingAction_ = UiAction::RetryFarewell;
            }
            else if (event == Hardware::InputEvent::LongPress) {
                cancelTo(ScreenId::DeathOptions);
            }
            break;

        case ScreenId::Boot:
        case ScreenId::HatchTransition:
        case ScreenId::GrowTransition:
        case ScreenId::DeathAnimation:
        case ScreenId::FarewellAnimation:
            break;
    }

    // Only idle Home can deliver a reminder: no input, care action, celebration,
    // sleep, or game audio is interrupted. Recheck live needs on each update.
    if (careReminderPending_ && sad_ && screen_ == ScreenId::Home &&
        pet_.sleepMode() == Pet::SleepMode::Awake &&
        !feeding_ && !cleaning_ && !treating_ && !careLevelUpPending_ &&
        !levelUpActive_ && pendingAction_ == UiAction::None &&
        event == Hardware::InputEvent::None && !sound_.isPlaying() &&
        (!careReminderPlayed_ || static_cast<uint32_t>(now - lastCareReminderAt_) >= kCareReminderCooldownMs)) {
        sound_.playCareReminder();
        careReminderPending_ = false;
        careReminderPlayed_ = true;
        lastCareReminderAt_ = now;
    }
    if (kDebugUiEvents && menuIndex_ != previousMenuIndex) {
        Serial.printf("Menu index: %u -> %u\n", previousMenuIndex, menuIndex_);
    }
    return screen_ != previousScreen || homeFocus_ != previousHomeFocus ||
           menuIndex_ != previousMenuIndex || statusPage_ != previousStatusPage ||
           deathOptionIndex_ != previousDeathOptionIndex ||
           memorialIndex_ != previousMemorialIndex ||
           memorialCategoryIndex_ != previousMemorialCategoryIndex ||
           deleteConfirmIndex_ != previousDeleteConfirmIndex;
}

void UiController::render() {
    updateSadState();
    if ((screen_ == ScreenId::Home || screen_ == ScreenId::Sleeping ||
         screen_ == ScreenId::DetailedStatus ||
         screen_ == ScreenId::FeedCare || screen_ == ScreenId::CleanCare ||
         screen_ == ScreenId::PlayCare) &&
        pet_.displayRevision() != lastRenderedPetRevision_) dirty_ = true;
    if (!dirty_ || !display_.isInitialized()) return;

    if (levelUpActive_ && screen_ != ScreenId::PlayCare) renderLevelUp();
    else switch (screen_) {
        case ScreenId::FarewellInfo:
        case ScreenId::FarewellConfirm:
        case ScreenId::FarewellAnimation:
        case ScreenId::FarewellDone:
        case ScreenId::FarewellBlocked: renderFarewell(); break;
        case ScreenId::Boot: renderBoot(); break;
        case ScreenId::Home: renderHome(); break;
        case ScreenId::MainMenu: renderMainMenu(); break;
        case ScreenId::Volume: renderVolume(); break;
        case ScreenId::FeedCare: renderCare("餵食", "飽食", pet_.satiety()); break;
        case ScreenId::CleanCare: renderCare("清潔", "清潔", pet_.cleanliness()); break;
        case ScreenId::GameSelect: renderGameSelect(); break;
        case ScreenId::PlayCare: renderGame(); break;
        case ScreenId::Rest: renderRest(); break;
        case ScreenId::Sleeping: renderSleeping(); break;
        case ScreenId::DetailedStatus: renderDetailedStatus(); break;
        case ScreenId::HatchTransition: renderHatchTransition(); break;
        case ScreenId::GrowTransition: renderGrowTransition(); break;
        case ScreenId::DeathAnimation: renderDeathAnimation(); break;
        case ScreenId::DeathMemorial: renderDeathMemorial(); break;
        case ScreenId::DeathOptions: renderDeathOptions(); break;
        case ScreenId::MemorialCategories: renderMemorialCategories(); break;
        case ScreenId::MemorialHelp: renderMemorialHelp(); break;
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
        case ScreenId::FarewellInfo: return "FAREWELL_INFO";
        case ScreenId::FarewellConfirm: return "FAREWELL_CONFIRM";
        case ScreenId::FarewellAnimation: return "FAREWELL_ANIMATION";
        case ScreenId::FarewellDone: return "FAREWELL_DONE";
        case ScreenId::FarewellBlocked: return "FAREWELL_BLOCKED";
        case ScreenId::Boot: return "BOOT";
        case ScreenId::Home: return "HOME";
        case ScreenId::MainMenu: return "MAIN_MENU";
        case ScreenId::Volume: return "SOUND";
        case ScreenId::FeedCare: return "FEED_CARE";
        case ScreenId::CleanCare: return "CLEAN_CARE";
        case ScreenId::GameSelect: return "GAME_SELECT";
        case ScreenId::PlayCare: return "PLAY_CARE";
        case ScreenId::Rest: return "REST";
        case ScreenId::Sleeping: return "SLEEPING";
        case ScreenId::DetailedStatus: return "DETAILED_STATUS";
        case ScreenId::HatchTransition: return "HATCH_TRANSITION";
        case ScreenId::GrowTransition: return "GROW_TRANSITION";
        case ScreenId::DeathAnimation: return "DEATH_ANIMATION";
        case ScreenId::DeathMemorial: return "DEATH_MEMORIAL";
        case ScreenId::DeathOptions: return "DEATH_OPTIONS";
        case ScreenId::MemorialCategories: return "MEMORIAL_CATEGORIES";
        case ScreenId::MemorialHelp: return "MEMORIAL_HELP";
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

void UiController::beginTreatment(uint32_t now) {
    if (pet_.isDead() || pet_.lifeStage() == Pet::LifeStage::Egg ||
        !pet_.isSick() || pet_.sleepMode() != Pet::SleepMode::Awake) {
        showTreatmentNotice(now);
        return;
    }
    setScreen(ScreenId::Home);
    treating_ = true;
    treatmentActionQueued_ = treatmentSucceeded_ = false;
    treatmentStartedAt_ = now;
    treatmentElapsedMs_ = 0;
    treatmentVisualStage_ = pet_.lifeStage();
    dirty_ = true;
}

void UiController::onCleanSucceeded(uint32_t now) {
    setScreen(ScreenId::Home);
    cleaning_ = true;
    cleaningStartedAt_ = now;
    cleaningFrame_ = 0;
    dirty_ = true;
}

void UiController::showTreatmentNotice(uint32_t now) {
    setScreen(ScreenId::MainMenu);
    menuIndex_ = 2;
    treatmentNotice_ = true;
    treatmentNoticeStartedAt_ = now;
    dirty_ = true;
}

void UiController::onTreatResult(bool success, uint32_t now) {
    if (!treating_) return;
    if (success) {
        treatmentSucceeded_ = true;
        // Anchor the empty bottle / recovery phases to the real settlement.
        treatmentStartedAt_ = now - kTreatmentPourDoneMs;
        treatmentElapsedMs_ = kTreatmentPourDoneMs;
    } else {
        treating_ = false;
        showTreatmentNotice(now);
    }
    dirty_ = true;
}

void UiController::onSleepStarted(uint32_t now) {
    normalSleepFailed_ = false;
    updateSadState();
    sleepStartedAtMs_ = now;
    sleepAnimationFrame_ = 0;
    sleepZPhase_ = 0;
    sleepVisualStage_ = pet_.lifeStage();
    setScreen(ScreenId::Sleeping);
}

void UiController::onNormalSleepFailed() {
    normalSleepFailed_ = true;
    dirty_ = true;
}

void UiController::setStorageStatus(bool saveFailed, bool capacityLow) {
    if (storageSaveFailed_ == saveFailed && storageLow_ == capacityLow) return;
    storageSaveFailed_ = saveFailed;
    storageLow_ = capacityLow;
    dirty_ = true;
}

void UiController::onWakeSucceeded() {
    updateSadState();
    if (sad_) careReminderPending_ = true;
    setScreen(ScreenId::Home);
}

void UiController::onDeepSleepPending(bool pending, bool failed) {
    deepSleepPending_ = pending;
    deepSleepFailed_ = failed;
    dirty_ = true;
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
    if (!matchesMemorial(memorialIndex_)) selectMemorial(1);
    setScreen(ScreenId::Graveyard);
}

void UiController::beginGrowthTransition(ScreenId screen, uint32_t now) {
    if (levelUpActive_ && screen_ != ScreenId::PlayCare) careLevelUpPending_ = true;
    levelUpActive_ = false;
    pendingAction_ = UiAction::None;
    growthTransitionStartedAt_ = now;
    growthTransitionElapsedMs_ = 0;
    growthTransitionFrame_ = 0;
    setScreen(screen);
    if (screen == ScreenId::HatchTransition) sound_.playHatch();
    else sound_.playSuccess();
}

void UiController::beginDeathAnimation(uint32_t now) {
    careLevelUpPending_ = levelUpActive_ = false;
    pendingAction_ = UiAction::None;
    deathStartedAt_ = now;
    deathAnimationElapsedMs_ = 0;
    deathAnimationFrame_ = 0;
    setScreen(ScreenId::DeathAnimation);
    sound_.playDeath();
}

// Navigation feedback runs after cleanup, which may stop an audition and
// restore the saved sound mode. Automatic transitions use silent setScreen().
void UiController::confirmTo(ScreenId screen) {
    setScreen(screen);
    sound_.playConfirm();
}

void UiController::cancelTo(ScreenId screen) {
    setScreen(screen);
    sound_.playCancel();
}

void UiController::setScreen(ScreenId screen) {
    if (screen_ == screen) return;
    if (screen_ == ScreenId::Volume) {
        // Saving updates originalVolume_ first. All other exits, including
        // death/growth interruptions, discard the unconfirmed audition.
        sound_.stopTone();
        sound_.setVolume(originalVolume_);
        pendingAction_ = UiAction::None;
    }
    treatmentNotice_ = false;
    if (screen_ == ScreenId::PlayCare) {
        levelUpActive_ = false;
        game_.cancel();
        memoryNotes_.cancel();
        if (pendingAction_ == UiAction::FinishGame) pendingAction_ = UiAction::None;
    }
    screen_ = screen;
    feeding_ = false;
    cleaning_ = false;
    treating_ = false;
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
    const bool treatmentSparkling = treating_ && treatmentSucceeded_ &&
        treatmentElapsedMs_ >= kTreatmentSparkleStartMs;
    // The cure settles before the sparkle starts; retain sad through that gap.
    const bool sad = treating_ ? !treatmentSparkling : sad_;
    // Care effects hide dirt temporarily; restore it from live cleanliness.
    if (!treating_ && !cleaning_ && pet_.lifeStage() != Pet::LifeStage::Egg) {
        PetIcons::drawDirt(display_, pet_.cleanlinessState());
    }
    if (pet_.lifeStage() != Pet::LifeStage::Egg) {
        PetIcons::drawMood(display_, 3, 3, pet_.moodState());
        PetIcons::drawHunger(display_, 3, 22, pet_.hungerState());
    }

    if (cleaning_) {
        display_.drawGlyph(23, 3, PetIcons::kCleaningFrames[cleaningFrame_],
                           PetIcons::kCleaningWidth, PetIcons::kCleaningHeight);
    } else if (feeding_) {
        PetIcons::drawEatingPet(display_, 32, 6, pet_.lifeStage(),
                               feedingFrame_, feedingBowlStage_, pet_.speciesId());
    } else if (sad && pet_.lifeStage() != Pet::LifeStage::Egg) {
        PetIcons::drawSadPet(display_, 32, 6,
                            treating_ ? treatmentVisualStage_ : pet_.lifeStage(),
                            treating_ ? 0 : homeAnimationFrame_, pet_.speciesId(), !treating_);
    } else {
        PetIcons::drawPet(display_, 32, 6,
                          treating_ ? treatmentVisualStage_ : pet_.lifeStage(),
                          eggCrackStage_, treating_ ? 0 : homeAnimationFrame_, pet_.speciesId());
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
    if (treating_) {
        if (!treatmentSparkling) {
            const uint8_t frame = treatmentElapsedMs_ < 250 ? 0 :
                treatmentElapsedMs_ < 500 ? 1 : treatmentElapsedMs_ < 850 ? 2 :
                treatmentElapsedMs_ < kTreatmentPourDoneMs ? 3 : 4;
            const int16_t potionX = pet_.speciesId() == Pet::SpeciesId::Wyvern &&
                treatmentVisualStage_ == Pet::LifeStage::Adult ? 88 : 78;
            display_.drawGlyph(potionX, 1, PetIcons::kPotionFrames[frame],
                               PetIcons::kPotionWidth, PetIcons::kPotionHeight);
        } else {
            const uint8_t frame = (treatmentElapsedMs_ - kTreatmentSparkleStartMs) / 125;
            drawSparkles(display_, frame, 33, 94);
        }
    }
}

void UiController::renderPetFooter() {
    // A fixed 3px outline shows 100%; the 1px center line shows earned EXP.
    char levelText[6];  // "Lv255" plus terminator covers the uint8_t level.
    if (pet_.lifeStage() == Pet::LifeStage::Egg) strcpy(levelText, "EGG");
    else if (pet_.lifeStage() == Pet::LifeStage::Baby) strcpy(levelText, "BABY");
    else snprintf(levelText, sizeof(levelText), "Lv%u", static_cast<unsigned>(pet_.level()));
    const int16_t levelX = 128 - static_cast<int16_t>(strlen(levelText) * 5);
    constexpr int16_t kFrameX = 3;
    constexpr int16_t kFrameY = 59;
    constexpr int16_t kFrameWidth = 99;  // Ends at x=101, 6px before "BABY"/"Lv20".
    constexpr int16_t kFillWidth = kFrameWidth - 2;
    const uint16_t threshold = pet_.expToNextLevel();
    const int16_t filled = threshold == 0 ?
        (pet_.lifeStage() == Pet::LifeStage::Egg ? 0 : kFillWidth) :
        static_cast<uint32_t>(kFillWidth) * pet_.exp() / threshold;
    display_.drawFrame(kFrameX, kFrameY, kFrameWidth, 3);
    if (filled > 0) display_.drawLine(kFrameX + 1, kFrameY + 1,
                                      kFrameX + filled, kFrameY + 1);
    display_.drawSmallText(levelX, 63, levelText);
}

void UiController::onCareRewardApplied(uint8_t previousLevel, uint32_t now) {
    if (pet_.level() <= previousLevel) return;
    levelUpPreviousLevel_ = previousLevel;
    levelUpFinalLevel_ = pet_.level();
    careRewardAt_ = now;
    careLevelUpPending_ = true;
    dirty_ = true;
}

void UiController::renderMainMenu() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawUiText(9, 14, storageSaveFailed_ ?
        (storageLow_ ? "存檔空間不足" : "存檔失敗") :
        storageLow_ ? "存檔空間偏低" : "選單");
    char pageText[4];
    snprintf(pageText, sizeof(pageText), "%u/3",
             static_cast<unsigned>(menuIndex_ / 3 + 1));
    display_.drawText(99, 14, pageText);
    display_.drawLine(6, 18, 121, 18);
    const uint8_t pageStart = (menuIndex_ / 3) * 3;
    for (uint8_t row = 0; row < 3; ++row) {
        const uint8_t i = pageStart + row;
        if (i >= kMenuItemCount) break;
        const int16_t baseline = 32 + (row * 14);
        if (i == menuIndex_) display_.drawSmallText(8, baseline, ">");
        display_.drawUiText(22, baseline,
                            i == 2 && treatmentNotice_ ? "不需治療" : kMenuLabels[i]);
    }
}

void UiController::beginVolume(ScreenId returnScreen) {
    volumeReturnScreen_ = returnScreen;
    originalVolume_ = sound_.volume();
    volumeSaveFailed_ = false;
    volumeButtonsFocused_ = false;
    volumeCancelSelected_ = false;
    sound_.stopTone();
    confirmTo(ScreenId::Volume);
}

void UiController::onVolumeSaveResult(bool success) {
    if (screen_ != ScreenId::Volume) return;
    if (success) {
        originalVolume_ = sound_.volume();
        confirmTo(volumeReturnScreen_);
    } else {
        volumeSaveFailed_ = true;
        dirty_ = true;
        sound_.playFailure();
    }
}

void UiController::renderVolume() {
    display_.clear();
    display_.drawFrame(48, 21, 8, 12);
    display_.drawLine(55, 21, 65, 11);
    display_.drawLine(65, 11, 65, 42);
    display_.drawLine(65, 42, 55, 32);
    if (!sound_.volume()) {
        display_.drawLine(69, 22, 79, 32);
        display_.drawLine(79, 22, 69, 32);
    }
    else {
        display_.drawLine(69, 20, 73, 24);
        display_.drawLine(73, 24, 73, 29);
        display_.drawLine(73, 29, 69, 33);
        display_.drawLine(73, 15, 79, 21);
        display_.drawLine(79, 21, 79, 32);
        display_.drawLine(79, 32, 73, 38);
    }
    if (!volumeButtonsFocused_) {
        display_.drawText(32, 31, "<");
        display_.drawText(90, 31, ">");
    }
    // A failed save leaves both buttons available, relabeling confirm as retry.
    display_.drawUiText(34, 58, volumeSaveFailed_ ? "重試" : "確定");
    display_.drawUiText(82, 58, "取消");
    if (volumeButtonsFocused_)
        display_.drawSmallText(volumeCancelSelected_ ? 70 : 22, 58, ">");
}

void UiController::renderDetailedStatus() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(64 - static_cast<int16_t>(strlen(pet_.name()) * 3),
                      14, pet_.name());
    char pageText[4];
    snprintf(pageText, sizeof(pageText), "%u/4", static_cast<unsigned>(statusPage_ + 1));
    display_.drawText(99, 14, pageText);
    display_.drawLine(6, 18, 121, 18);
    if (statusPage_ == 0) {
        renderDetailedStatusPage1();
    } else if (statusPage_ == 1) {
        renderDetailedStatusPage2();
    } else if (statusPage_ == 2) {
        // Actual lit rows: divider 18, level 24..34, EXP 40..46,
        // progress frame 52..58. Each adjacent pair has five blank rows.
        display_.drawStatusText(kStatusLabelX, 34, "等級");
        drawStatusNumber(display_, 34, static_cast<unsigned>(pet_.level()));
        char progress[24];
        display_.drawText(kStatusLabelX, 47, "EXP");
        if (!pet_.expToNextLevel()) strcpy(progress, "MAX");
        else snprintf(progress, sizeof(progress), "%u/%u", pet_.exp(), pet_.expToNextLevel());
        display_.drawText(kStatusValueRight - static_cast<int16_t>(strlen(progress) * 6),
                          47, progress);
        display_.drawFrame(9, 52, 110, 7);
        const uint16_t fill = pet_.expToNextLevel() ?
            static_cast<uint32_t>(pet_.exp()) * 106 / pet_.expToNextLevel() : 106;
        if (fill) for (int16_t y = 54; y <= 56; ++y)
            display_.drawLine(11, y, 10 + fill, y);
    } else {
        // Chinese rows span y=24..35 and y=44..55; values end at x=118.
        const uint64_t age = pet_.ageSeconds();
        char days[32];
        if (age / 86400 > 999999) strcpy(days, ">999999 天");
        else snprintf(days, sizeof(days), "%llu 天",
                      static_cast<unsigned long long>(age / 86400));
        char time[32];
        snprintf(time, sizeof(time), "%u 時 %u 分",
                 static_cast<unsigned>((age / 3600) % 24),
                 static_cast<unsigned>((age / 60) % 60));
        display_.drawUiText(kStatusLabelX, 35, "年齡");
        display_.drawUiText(kStatusValueRight - display_.uiTextWidth(days), 35, days);
        display_.drawUiText(kStatusValueRight - display_.uiTextWidth(time), 55, time);
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
    display_.drawStatusText(kStatusLabelX, 35, "健康");
    drawStatusTextValue(display_, 35, pet_.isSick() ? "生病" : "正常");

    display_.drawUiText(kStatusLabelX, 55, "階段");
    const char* stage = pet_.lifeStage() == Pet::LifeStage::Egg ? "蛋" :
        pet_.lifeStage() == Pet::LifeStage::Baby ? "幼年" : "成年";
    display_.drawUiText(kStatusValueRight - display_.uiTextWidth(stage), 55, stage);
}

void UiController::renderPlaceholder(const char* title) {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 15, title);
    display_.drawLine(6, 20, 121, 20);
    drawCenteredUiText(display_, 41, "尚未完成");
    drawCenteredUiText(display_, 59, "長按返回");
}

uint8_t UiController::takeGameReward() {
    return selectedGame_ == 1 ? memoryNotes_.takeReward() : game_.takeReward();
}
uint8_t UiController::gameExpReward() const {
    return selectedGame_ == 1 ? memoryNotes_.expReward() : game_.expReward();
}

void UiController::onGameRewardApplied(uint8_t actualGain, uint16_t expGain,
                                       uint8_t previousLevel) {
    gameMoodGain_ = actualGain;
    gameExpGain_ = expGain;
    gamePreviousLevel_ = previousLevel;
    gameFinalLevel_ = pet_.level();
    dirty_ = true;
}

void UiController::updateGame(Hardware::InputEvent event, uint32_t now) {
    using namespace Games;
    if (event == Hardware::InputEvent::LongPress) {
        cancelTo(ScreenId::MainMenu);
        return;
    }
    if (pet_.lifeStage() == Pet::LifeStage::Egg || pet_.isSick() ||
        pet_.sleepMode() != Pet::SleepMode::Awake) {
        levelUpActive_ = false;
        if (game_.phase() != RpsPhase::Ready) { game_.begin(); dirty_ = true; }
        if (event == Hardware::InputEvent::Press) sound_.playFailure();
        return;
    }
    if (levelUpActive_) {
        const uint32_t elapsed = now - levelUpStartedAt_;
        if (elapsed >= 2000) levelUpActive_ = false;
        levelUpFrame_ = static_cast<uint8_t>(elapsed / 125);
        dirty_ = true;
        return; // Consume this event rather than selecting replay after the animation.
    }
    RpsInput input = RpsInput::None;
    switch (event) {
        case Hardware::InputEvent::Left: input = RpsInput::Left; break;
        case Hardware::InputEvent::Right: input = RpsInput::Right; break;
        case Hardware::InputEvent::Press: input = RpsInput::Confirm; break;
        case Hardware::InputEvent::Up:
        case Hardware::InputEvent::Down: input = RpsInput::ToggleReplay; break;
        default: break;
    }
    const RpsPhase previous = game_.phase();
    const uint8_t previousDigit = game_.countdown();
    if (game_.update(input, now)) dirty_ = true;
    const RpsPhase current = game_.phase();
    if (current == RpsPhase::Inactive) {
        cancelTo(ScreenId::MainMenu);
        return;
    }
    if (current == RpsPhase::Select && previous != current && game_.round() == 1) {
        gameMoodGain_ = 0;
        gameExpGain_ = 0;
        gamePreviousLevel_ = gameFinalLevel_ = 0;
    }
    if (previous == RpsPhase::Summary && current == RpsPhase::Replay &&
        gamePreviousLevel_ && gameFinalLevel_ > gamePreviousLevel_) {
        levelUpActive_ = true;
        levelUpStartedAt_ = now;
        levelUpFrame_ = 0;
        levelUpPreviousLevel_ = gamePreviousLevel_;
        levelUpFinalLevel_ = gameFinalLevel_;
        sound_.playLevelUp();
    }
    if (current == RpsPhase::Countdown &&
        (previous != current || previousDigit != game_.countdown())) sound_.playTone(1000, 45);
    if (current == RpsPhase::Reveal && previous != current) {
        if (game_.roundOutcome() == RpsOutcome::Win) sound_.playSuccess();
        else if (game_.roundOutcome() == RpsOutcome::Loss) sound_.playFailure();
        else sound_.playConfirm();
    }
    if (game_.rewardPending()) pendingAction_ = UiAction::FinishGame;
}

void UiController::renderGameSelect() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 15, "陪玩");
    display_.drawLine(6, 20, 121, 20);
    display_.drawUiText(40, 36, "猜拳");
    display_.drawUiText(40, 56, "記憶音符");
    display_.drawText(26, selectedGame_ ? 56 : 36, ">");
}

void UiController::updateMemoryNotes(Hardware::InputEvent event, uint32_t now, bool neutral) {
    using namespace Games;
    if (event == Hardware::InputEvent::LongPress) {
        cancelTo(ScreenId::MainMenu);
        return;
    }
    if (pet_.lifeStage() == Pet::LifeStage::Egg || pet_.isSick() ||
        pet_.sleepMode() != Pet::SleepMode::Awake) {
        if (memoryNotes_.phase() != NotesPhase::Ready) {
            memoryNotes_.begin();
            sound_.stopTone();
            if (pendingAction_ == UiAction::FinishGame) pendingAction_ = UiAction::None;
            dirty_ = true;
        }
        levelUpActive_ = false;
        if (event == Hardware::InputEvent::Press) sound_.playFailure();
        return;
    }
    if (levelUpActive_) {
        const uint32_t elapsed = now - levelUpStartedAt_;
        if (elapsed >= 2000) levelUpActive_ = false;
        levelUpFrame_ = static_cast<uint8_t>(elapsed / 125);
        dirty_ = true;
        return;
    }
    NotesInput input = NotesInput::None;
    switch (event) {
        case Hardware::InputEvent::Left: input = NotesInput::Left; break;
        case Hardware::InputEvent::Up: input = NotesInput::Up; break;
        case Hardware::InputEvent::Right: input = NotesInput::Right; break;
        case Hardware::InputEvent::Down: input = NotesInput::Down; break;
        case Hardware::InputEvent::Press: input = NotesInput::Confirm; break;
        default: break;
    }
    const NotesPhase previous = memoryNotes_.phase();
    const NoteDirection previousActive = memoryNotes_.activeDirection();
    const uint8_t previousProgress = memoryNotes_.progress();
    if (memoryNotes_.update(input, now, neutral, !sound_.isPlaying())) dirty_ = true;
    const NotesPhase current = memoryNotes_.phase();
    if (current == NotesPhase::Inactive) {
        cancelTo(ScreenId::MainMenu);
        return;
    }
    if (current == NotesPhase::Playback && previous != current && memoryNotes_.round() == 1) {
        gameMoodGain_ = 0; gameExpGain_ = 0;
        gamePreviousLevel_ = gameFinalLevel_ = 0;
    }
    if (previous == NotesPhase::Summary && current == NotesPhase::Replay &&
        gamePreviousLevel_ && gameFinalLevel_ > gamePreviousLevel_) {
        levelUpActive_ = true;
        levelUpStartedAt_ = now;
        levelUpFrame_ = 0;
        levelUpPreviousLevel_ = gamePreviousLevel_;
        levelUpFinalLevel_ = gameFinalLevel_;
        sound_.playLevelUp();
    }
    const NoteDirection active = memoryNotes_.activeDirection();
    if (active != NoteDirection::None &&
        (active != previousActive || previousProgress != memoryNotes_.progress())) {
        const uint16_t notes[] = {523, 587, 659, 784};
        sound_.playTone(notes[static_cast<uint8_t>(active)],
                        current == NotesPhase::Playback ? MemoryNotes::kToneMs : MemoryNotes::kAnswerFlashMs);
    }
    if (current == NotesPhase::Feedback && previous != current) {
        if (memoryNotes_.roundSucceeded()) sound_.playSuccess();
        else sound_.playFailure();
    }
    if (memoryNotes_.rewardPending()) pendingAction_ = UiAction::FinishGame;
}

void UiController::renderMemoryNotes() {
    using namespace Games;
    if (levelUpActive_) { renderLevelUp(); return; }
    display_.clear();
    const auto arrow = [&](int16_t cx, int16_t cy, NoteDirection direction, bool active) {
        if (active) for (int16_t y = cy-6; y <= cy+6; ++y)
            display_.drawLine(cx-8,y,cx+8,y);
        // A nine-pixel filled arrow. Rotations preserve the approved geometry.
        for (int16_t y=-4; y<=4; ++y) {
            const int16_t half = y<=0 ? y+4 : 1;
            for (int16_t x=-half; x<=half; ++x) {
                int16_t xx=x, yy=y;
                if (direction == NoteDirection::Down) { xx=-x; yy=-y; }
                else if (direction == NoteDirection::Left) { xx=y; yy=-x; }
                else if (direction == NoteDirection::Right) { xx=-y; yy=x; }
                if (active) display_.clearArea(cx+xx,cy+yy,1,1);
                else display_.drawLine(cx+xx,cy+yy,cx+xx,cy+yy);
            }
        }
    };
    char text[32];
    if (pet_.lifeStage() == Pet::LifeStage::Egg || pet_.isSick()) {
        display_.drawFrame(0,0,128,64);
        drawCenteredUiText(display_,15,"記憶音符");
        display_.drawLine(6,20,121,20);
        drawCenteredUiText(display_,38,pet_.isSick()?"請先治療":"等待孵化");
        drawCenteredUiText(display_,58,"長按返回");
        return;
    }
    const NotesPhase phase=memoryNotes_.phase();
    if (phase == NotesPhase::Ready) {
        display_.drawFrame(0,0,128,64);
        drawCenteredUiText(display_,15,"記憶音符");
        display_.drawLine(6,20,121,20);
        drawCenteredUiText(display_,38,"記住箭頭順序");
        drawCenteredUiText(display_,58,"按下開始");
    } else if (phase == NotesPhase::Playback || phase == NotesPhase::Answer ||
               phase == NotesPhase::AnswerComplete) {
        display_.drawUiText(5,14,phase==NotesPhase::Playback?"記住旋律":"換你了");
        snprintf(text,sizeof(text),"%u/5",memoryNotes_.round());
        display_.drawText(104,14,text);
        display_.drawLine(4,18,123,18);
        const NoteDirection dirs[]={NoteDirection::Up,NoteDirection::Left,NoteDirection::Right,NoteDirection::Down};
        const int16_t xs[]={64,44,84,64}, ys[]={27,39,39,49};
        for (uint8_t i=0;i<4;++i) arrow(xs[i],ys[i],dirs[i],memoryNotes_.activeDirection()==dirs[i]);
        const uint8_t count=memoryNotes_.length();
        const int16_t start=64-(count*10-6)/2;
        for (uint8_t i=0;i<count;++i) {
            const int16_t x=start+i*10;
            display_.drawFrame(x,59,4,4);
            if (i<memoryNotes_.progress()) display_.drawFrame(x+1,60,2,2);
        }
    } else if (phase == NotesPhase::Feedback) {
        display_.drawFrame(0,0,128,64);
        drawCenteredUiText(display_,15,memoryNotes_.roundSucceeded()?"答對了":
                            memoryNotes_.timedOut()?"超時了":"再試一次");
        if (memoryNotes_.roundSucceeded()) {
            snprintf(text,sizeof(text),"%u/%u",memoryNotes_.length(),memoryNotes_.length());
            drawCenteredUiText(display_,37,text);
        } else {
            display_.drawUiText(14,38,"正確");
            arrow(84,33,memoryNotes_.expectedDirection(),true);
        }
        drawCenteredUiText(display_,59,"按下繼續");
    } else if (phase == NotesPhase::Summary) {
        display_.drawFrame(0,0,128,64);
        drawCenteredUiText(display_,14,"記憶音符");
        const char* const labels[]={"答對","心情","EXP"};
        const unsigned values[]={memoryNotes_.score(),gameMoodGain_,gameExpGain_};
        // User-approved framebuffer: title plus three rows, no table grid.
        const int16_t baselines[]={31,46,60};
        for (uint8_t row=0;row<3;++row) {
            const int16_t baseline=baselines[row];
            if (row==2) {
                display_.drawText(27,baseline,"E");
                display_.drawText(35,baseline,"X");
                display_.drawText(43,baseline,"P");
            } else display_.drawUiText(26,baseline,labels[row]);
            snprintf(text,sizeof(text),row==0?"%u/5":"+%u",values[row]);
            display_.drawText(101-static_cast<int16_t>(strlen(text)*6),baseline,text);
        }
    } else if (phase == NotesPhase::Replay) {
        display_.drawFrame(0,0,128,64);
        drawCenteredUiText(display_,15,"再玩？");
        display_.drawLine(6,20,121,20);
        display_.drawUiText(48,36,"再玩");
        display_.drawUiText(48,56,"返回");
        display_.drawText(34,memoryNotes_.replaySelected()?36:56,">");
    }
}

void UiController::renderGame() {
    using namespace Games;
    if (selectedGame_ == 1) { renderMemoryNotes(); return; }
    if (levelUpActive_) { renderLevelUp(); return; }
    const uint8_t* const icons[] = {RpsIcons::kScissors, RpsIcons::kRock, RpsIcons::kPaper};
    const char* const names[] = {"剪刀", "石頭", "布"};
    const auto outcomeText = [](RpsOutcome result) {
        return result == RpsOutcome::Win ? "你贏了" :
            result == RpsOutcome::Loss ? "你輸了" : "平手";
    };
    const auto hand = [&](int16_t x, int16_t y, RpsMove move) {
        display_.drawGlyph(x, y, icons[static_cast<unsigned>(move)],
                           RpsIcons::kWidth, RpsIcons::kHeight);
    };
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    if (pet_.lifeStage() == Pet::LifeStage::Egg || pet_.isSick()) {
        drawCenteredUiText(display_, 15, "猜拳");
        display_.drawLine(6, 20, 121, 20);
        drawCenteredUiText(display_, 38, pet_.isSick() ? "請先治療" : "等待孵化");
        drawCenteredUiText(display_, 58, "長按返回");
        return;
    }
    char text[32];
    switch (game_.phase()) {
        case RpsPhase::Ready:
            drawCenteredUiText(display_, 15, "猜拳");
            display_.drawLine(6, 20, 121, 20);
            drawCenteredUiText(display_, 38, "共三回合");
            drawCenteredUiText(display_, 58, "按下開始");
            break;
        case RpsPhase::Select:
            display_.drawUiText(6, 15, "猜拳");
            snprintf(text, sizeof(text), "%u/3", game_.round());
            display_.drawText(102, 15, text);
            for (uint8_t i = 0; i < 3; ++i) {
                hand(12 + 40 * i, 26, static_cast<RpsMove>(i));
                if (i == static_cast<uint8_t>(game_.player()))
                    display_.drawFrame(8 + 40 * i, 23, 32, 24);
            }
            drawCenteredUiText(display_, 60, names[static_cast<unsigned>(game_.player())]);
            break;
        case RpsPhase::Countdown: {
            display_.drawUiText(6, 15, "猜拳");
            snprintf(text, sizeof(text), "%u/3", game_.round());
            display_.drawText(102, 15, text);
            // Large 3x5 digits, each cell 4x4; no extra font dependency.
            const uint16_t digits[] = {0, 0x2C97, 0x73E7, 0x73CF};
            const uint16_t bits = digits[game_.countdown()];
            for (uint8_t row = 0; row < 5; ++row)
                for (uint8_t col = 0; col < 3; ++col)
                    if (bits & (1U << (14 - row * 3 - col)))
                        for (uint8_t dy = 0; dy < 4; ++dy)
                            display_.drawLine(58 + col * 4, 26 + row * 4 + dy,
                                              61 + col * 4, 26 + row * 4 + dy);
            break;
        }
        case RpsPhase::Reveal:
            display_.drawUiText(23, 15, "你");
            display_.drawText(94 - static_cast<int16_t>(strlen(pet_.name()) * 3), 15, pet_.name());
            hand(17, 25, game_.player());
            hand(82, 25, game_.opponent());
            display_.drawText(58, 37, "VS");
            drawCenteredUiText(display_, 60, outcomeText(game_.roundOutcome()));
            break;
        case RpsPhase::Summary:
            drawCenteredUiText(display_, 15, outcomeText(game_.outcome()));
            snprintf(text, sizeof(text), "%u:%u", game_.wins(), game_.losses());
            drawCenteredUiText(display_, 29, text);
            snprintf(text, sizeof(text), "心情 +%u", gameMoodGain_);
            drawCenteredUiText(display_, 45, text);
            snprintf(text, sizeof(text), "EXP +%u", gameExpGain_);
            drawCenteredUiText(display_, 60, text);
            break;
        case RpsPhase::Replay:
            drawCenteredUiText(display_, 15, "再玩？");
            display_.drawLine(6, 20, 121, 20);
            display_.drawUiText(48, 36, "再玩");
            display_.drawUiText(48, 56, "返回");
            display_.drawText(34, game_.replaySelected() ? 36 : 56, ">");
            break;
        case RpsPhase::Inactive: break;
    }
}

void UiController::renderLevelUp() {
    display_.clear();
    // Unframed celebration: title y=3..9, original sprite 64x44,
    // Visible adult pixels y=13..50, footer y=53..60. No sprite rescaling.
    display_.drawText(37, 10, "LEVEL UP!");
    const uint8_t phase = levelUpFrame_ % 8;
    const int16_t jump = phase == 2 || phase == 3 ? 2 :
                         phase == 1 || phase == 4 ? 1 : 0;
    const int16_t baseY = pet_.speciesId() == Pet::SpeciesId::Wyvern &&
        pet_.lifeStage() == Pet::LifeStage::Adult ? 9 : 7;
    PetIcons::drawPet(display_, 32, baseY - jump, pet_.lifeStage(), 0, phase / 4, pet_.speciesId());
    drawSparkles(display_, levelUpFrame_, 18, 109);
    char left[8], right[8];
    snprintf(left, sizeof(left), "Lv%u", levelUpPreviousLevel_);
    snprintf(right, sizeof(right), "Lv%u", levelUpFinalLevel_);
    display_.drawText(25, 60, left);
    display_.drawLine(58, 56, 69, 56);
    display_.drawLine(66, 53, 69, 56);
    display_.drawLine(66, 59, 69, 56);
    display_.drawText(78, 60, right);
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
    drawStatusNumber(display_, 35, value);
    const char* action = screen_ == ScreenId::FeedCare ? "按下執行 +20" :
        screen_ == ScreenId::CleanCare ? "按下執行 +30" :
        "按下執行";
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
    if (deepSleepPending_) {
        drawCenteredUiText(display_, 38, "Release SW");
        drawCenteredUiText(display_, 55, "Hold SW: cancel");
        return;
    }
    if (deepSleepFailed_ || normalSleepFailed_) {
        drawCenteredUiText(display_, 38, storageSaveFailed_ ?
            (storageLow_ ? "存檔空間不足" : "存檔失敗") : "入睡失敗");
        drawCenteredUiText(display_, 55, "長按返回");
        return;
    }
    display_.drawUiText(24, 36, "一般睡眠");
    display_.drawUiText(24, 52, "省電睡眠");
    display_.drawText(12, restOption_ == 0 ? 36 : 52, ">");
}

void UiController::renderSleeping() {
    display_.clear();
    PetIcons::drawMood(display_, 3, 3, pet_.moodState());
    PetIcons::drawHunger(display_, 3, 22, pet_.hungerState());
    PetIcons::drawSleepingPet(display_, 32, 6, sleepVisualStage_,
                              sleepAnimationFrame_, pet_.speciesId());
    if (pet_.isSick()) PetIcons::drawSick(display_, 113, 7);
    PetIcons::drawCleaningAlert(display_, 113, 23,
                                pet_.cleanlinessState());
    // Keep the familiar shortcut glyph, but omit the focus frame while asleep.
    PetIcons::drawStatusCard(display_, 113, 40);
    const char* zText = sleepZPhase_ == 0 ? "Z" :
        sleepZPhase_ == 1 ? "Zz" : "Zzz";
    const bool wyvern = pet_.speciesId() == Pet::SpeciesId::Wyvern;
    const bool baby = sleepVisualStage_ == Pet::LifeStage::Baby;
    display_.drawSmallText(wyvern ? (baby ? 76 : 83) : 78,
                          wyvern ? (baby ? 23 : 15) : 17, zText);
    PetIcons::drawDirt(display_, pet_.cleanlinessState());
    renderPetFooter();
}

void UiController::renderHatchTransition() {
    display_.clear();
    if (growthTransitionElapsedMs_ < 700) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Egg, 2, 0, pet_.speciesId());
    } else if (growthTransitionElapsedMs_ >= 1000) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Baby, 0,
                          growthTransitionFrame_ % 2, pet_.speciesId());
        char message[Pet::kPetNameMaxLength + sizeof(" 孵化了！")];
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
                          growthTransitionFrame_ % 2, pet_.speciesId());
    } else if (growthTransitionElapsedMs_ >= 1000) {
        PetIcons::drawPet(display_, 32, 2, Pet::LifeStage::Adult, 0,
                          growthTransitionFrame_ % 2, pet_.speciesId());
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
    PetIcons::drawPetDissolve(display_, 32, 14, pet_.lifeStage(), dissolveStage, pet_.speciesId());

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
    const char* options[] = {"查看紀念冊", "領養新蛋", "聲音"};
    for (uint8_t i = 0; i < 3; ++i) {
        const int16_t baseline = 32 + i * 14;
        if (deathOptionIndex_ == i) display_.drawSmallText(9, baseline, ">");
        display_.drawUiText(24, baseline, options[i]);
    }
}

bool UiController::matchesMemorial(uint8_t index) const {
    const auto* record = memorials_.at(index);
    return record && (memorialFilter_ == 0 ||
        (memorialFilter_ == 1 && record->kind == Storage::FarewellKind::Departed) ||
        (memorialFilter_ == 2 && record->kind == Storage::FarewellKind::Resting));
}

void UiController::selectMemorial(int direction) {
    int index = memorialIndex_ < memorials_.count() ? memorialIndex_ : (direction > 0 ? -1 : 0);
    for (uint8_t n = 0; n < memorials_.count(); ++n) {
        index = (index + direction + memorials_.count()) % memorials_.count();
        if (matchesMemorial(index)) { memorialIndex_ = index; dirty_ = true; return; }
    }
    memorialIndex_ = 255;
    dirty_ = true;
}

void UiController::renderMemorialCategories() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    const char* labels[] = {"遠行", "長眠", "說明"};
    for (uint8_t row = 0; row < 3; ++row) {
        const int16_t baseline = 18 + row * 18;
        if (row == memorialCategoryIndex_) display_.drawText(7, baseline, ">");
        display_.drawUiText(21, baseline, labels[row]);
        if (row < 2) {
            uint8_t count = 0;
            for (uint8_t i = 0; i < memorials_.count(); ++i) {
                const auto* record = memorials_.at(i);
                if ((row == 0 && record->kind == Storage::FarewellKind::Departed) ||
                    (row == 1 && record->kind == Storage::FarewellKind::Resting)) ++count;
            }
            char total[4];
            snprintf(total, sizeof(total), "%u", count);
            display_.drawSmallText(104, baseline, total);
        }
    }
}

void UiController::renderMemorialHelp() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    drawCenteredUiText(display_, 14, "說明");
    display_.drawLine(6, 18, 121, 18);
    display_.drawUiText(6, 32, "左右：切換紀錄");
    display_.drawUiText(6, 46, "短按：刪除該筆");
    display_.drawUiText(6, 60, "長按：返回上一頁");
}

void UiController::renderGraveyard() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawUiText(5, 14, "紀念冊");
    const char* filters[] = {"全部", "遠行", "長眠"};
    display_.drawUiText(55, 14, filters[memorialFilter_]);
    uint8_t count = 0, position = 0;
    for (uint8_t i = 0; i < memorials_.count(); ++i) {
        if (matchesMemorial(i)) { ++count; if (i == memorialIndex_) position = count; }
    }
    char page[8];
    snprintf(page, sizeof(page), "%u/%u", position, count);
    display_.drawSmallText(98, 14, page);
    display_.drawLine(6, 18, 121, 18);
    if (!matchesMemorial(memorialIndex_)) {
        drawCenteredUiText(display_, 40, "尚無紀錄");
        drawCenteredUiText(display_, 60, "長按返回");
        return;
    }
    const auto* memorial = memorials_.at(memorialIndex_);
    const bool departed = memorial->kind == Storage::FarewellKind::Departed;
    const bool hasPortrait = departed ||
        memorial->stage == static_cast<uint8_t>(Pet::LifeStage::Baby) ||
        memorial->stage == static_cast<uint8_t>(Pet::LifeStage::Adult);
    if (hasPortrait) {
        PetIcons::drawMemorialPet(display_, static_cast<Pet::LifeStage>(memorial->stage), !departed, memorial->speciesId);
    } else {
        PetIcons::drawTombstone(display_, 7, 23, 37, 37);
        display_.drawSmallText(18, 38, "RIP");
    }
    // Center two text rows in the space beside the portrait/tombstone.
    const int16_t textLeft = hasPortrait ? 73 : 53;
    const int16_t textWidth = 121 - textLeft;
    const int16_t nameWidth = strlen(memorial->name) * 6;
    display_.drawText(textLeft + (textWidth - nameWidth) / 2, 37, memorial->name);
    // Short duration label keeps the 64-pixel portrait and text separate.
    char ageText[14];
    formatAge(memorial->ageSeconds, ageText);
    const int16_t ageWidth = strlen(ageText + 4) * 5;
    display_.drawSmallText(textLeft + (textWidth - ageWidth) / 2, 51, ageText + 4);
}

void UiController::onFarewellResult(bool success, uint32_t now) {
    if (!success) {
        setScreen(pet_.isDead() ? ScreenId::AdoptionBlocked : ScreenId::FarewellBlocked);
    } else if (pet_.isDeparted()) {
        careLevelUpPending_ = levelUpActive_ = false;
        farewellStartedAt_ = now;
        farewellElapsedMs_ = 0;
        setScreen(ScreenId::FarewellAnimation);
    } else setScreen(ScreenId::DeathOptions);
    dirty_ = true;
}

void UiController::renderFarewell() {
    display_.clear();
    if (screen_ == ScreenId::FarewellDone) {
        PetIcons::drawCenteredPostcardPet(display_, pet_.lifeStage(), pet_.speciesId());
        display_.drawFrame(2, 2, 124, 60);
        display_.drawLine(64, 8, 64, 55);
        display_.drawText(75, 35, pet_.name());
        display_.drawLine(75, 38, 120, 38);
        display_.drawUiText(75, 51, "遠行");
        display_.drawLine(75, 55, 120, 55);
        // Perforated 13x16 postage stamp, lowered four pixels from the first draft.
        display_.drawFrame(108, 7, 13, 16);
        for (int16_t x = 109; x < 120; x += 3) {
            display_.clearArea(x, 7, 1, 1);
            display_.clearArea(x, 22, 1, 1);
        }
        for (int16_t y = 8; y < 22; y += 3) {
            display_.clearArea(108, y, 1, 1);
            display_.clearArea(120, y, 1, 1);
        }
        display_.drawLine(114, 11, 114, 18);
        display_.drawLine(111, 14, 117, 14);
        for (int16_t x : {112, 116}) for (int16_t y : {12, 16}) display_.drawLine(x, y, x, y);
        return; // The postcard remains until an explicit press.
    }
    if (screen_ == ScreenId::FarewellAnimation) {
        if (farewellElapsedMs_ >= kFarewellPoseMs) {
            for (int16_t y = 0; y < 64; ++y) display_.drawLine(0, y, 127, y);
        } else {
            PetIcons::drawPet(display_, 32, 0, pet_.lifeStage(), 0,
                             farewellElapsedMs_ >= kBirdIdleSecondFrameAtMs ? 1 : 0, pet_.speciesId());
            display_.drawText(64 - static_cast<int16_t>(strlen(pet_.name()) * 3), 57, pet_.name());
            for (int16_t x : {3, 124}) for (int16_t y : {3, 61}) {
                display_.drawLine(x, y, x + (x == 3 ? 10 : -10), y);
                display_.drawLine(x, y, x, y + (y == 3 ? 8 : -8));
            }
        }
        return;
    }
    display_.drawFrame(0, 0, 128, 64);
    if (screen_ == ScreenId::FarewellBlocked) {
        const bool full = !pet_.isDeparted() && memorials_.count() >= Storage::kMemorialLimit;
        drawCenteredUiText(display_, 22, full ? "紀錄已滿" : "保存未完成");
        drawCenteredUiText(display_, 42, full ? "請先刪除" : "按下重試");
        if (!pet_.isDeparted()) drawCenteredUiText(display_, 60, "長按返回");
    } else if (screen_ == ScreenId::FarewellConfirm) {
        drawCenteredUiText(display_, 20, "送別後無法召回");
        display_.drawText(64 - strlen(pet_.name()) * 3, 37, pet_.name());
        display_.drawUiText(22, 57, "留下");
        display_.drawUiText(82, 57, "送別");
        display_.drawText(farewellConfirmIndex_ == 0 ? 12 : 72, 57, ">");
    } else {
        drawCenteredUiText(display_, 16, "送牠出發旅行？");
        display_.drawText(64 - strlen(pet_.name()) * 3, 32, pet_.name());
        char age[14]; formatAge(pet_.ageSeconds(), age);
        display_.drawSmallText(64 - strlen(age) * 5 / 2, 45, age);
        drawCenteredUiText(display_, 61, "按下繼續");
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
