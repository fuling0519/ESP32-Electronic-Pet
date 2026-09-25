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
constexpr uint32_t kDeathAnimationDurationMs = 3600;
constexpr uint32_t kDeathAnimationFrameMs = 100;
constexpr uint8_t kMenuItemCount = 6;
constexpr bool kDebugUiEvents = true;
const char* const kMenuItems[kMenuItemCount] = {
    "Feed", "Clean", "Treat", "Play", "Rest", "Status"
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
    deathStartedAt_ = 0;
    deathAnimationElapsedMs_ = 0;
    deathAnimationFrame_ = 0;
    dirty_ = true;
    pendingAction_ = UiAction::None;
    lastRenderedPetRevision_ = pet_.displayRevision();
}

bool UiController::update(Hardware::InputEvent event, uint32_t now) {
    const ScreenId previousScreen = screen_;
    const HomeFocus previousHomeFocus = homeFocus_;
    const uint8_t previousMenuIndex = menuIndex_;
    const uint8_t previousStatusPage = statusPage_;

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

    switch (screen_) {
        case ScreenId::Home:
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
            } else if (event == Hardware::InputEvent::Press) {
                sound_.playConfirm();
                switch (menuIndex_) {
                    case 0: setScreen(ScreenId::FeedCare); break;
                    case 1: setScreen(ScreenId::CleanCare); break;
                    case 2: setScreen(ScreenId::TreatCare); break;
                    case 3: setScreen(ScreenId::PlayPlaceholder); break;
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
            if (event == Hardware::InputEvent::Press) {
                pendingAction_ = screen_ == ScreenId::FeedCare ? UiAction::Feed :
                    screen_ == ScreenId::CleanCare ? UiAction::Clean : UiAction::Treat;
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::MainMenu);
            }
            break;

        case ScreenId::PlayPlaceholder:
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
         screen_ == ScreenId::TreatCare) &&
        pet_.displayRevision() != lastRenderedPetRevision_) dirty_ = true;
    if (!dirty_ || !display_.isInitialized()) return;

    switch (screen_) {
        case ScreenId::Boot: renderBoot(); break;
        case ScreenId::Home: renderHome(); break;
        case ScreenId::MainMenu: renderMainMenu(); break;
        case ScreenId::FeedCare: renderCare("FEED", "Satiety", pet_.satiety()); break;
        case ScreenId::CleanCare: renderCare("CLEAN", "Clean", pet_.cleanliness()); break;
        case ScreenId::TreatCare:
            renderCare("TREAT", "Sick", pet_.isSick() ? 1 : 0); break;
        case ScreenId::PlayPlaceholder: renderPlaceholder("PLAY"); break;
        case ScreenId::RestPlaceholder: renderPlaceholder("REST"); break;
        case ScreenId::DetailedStatus: renderDetailedStatus(); break;
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
        case ScreenId::PlayPlaceholder: return "PLAY_PLACEHOLDER";
        case ScreenId::RestPlaceholder: return "REST_PLACEHOLDER";
        case ScreenId::DetailedStatus: return "DETAILED_STATUS";
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

void UiController::beginDeathAnimation(uint32_t now) {
    pendingAction_ = UiAction::None;
    deathStartedAt_ = now;
    deathAnimationElapsedMs_ = 0;
    deathAnimationFrame_ = 0;
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
    PetIcons::drawMood(display_, 3, 3, pet_.moodState());
    PetIcons::drawHunger(display_, 3, 22, pet_.hungerState());

    // 44 x 32 slime within the 20..108 / 0..54 stage.
    PetIcons::drawPetPlaceholder(display_, 43, 12);
    if (pet_.isSick()) PetIcons::drawSick(display_, 113, 7);
    PetIcons::drawCleaningAlert(display_, 113, 23, pet_.cleanlinessState());
    PetIcons::drawStatusCard(display_, 113, 40);
    if (homeFocus_ == HomeFocus::Status) {
        // One-pixel breathing room keeps the selection frame distinct from
        // the card's own outline without changing the HUD layout.
        display_.drawFrame(111, 38, 16, 16);
    }

    // Reserve y=55 as whitespace. This baseline is not EXP progress.
    char levelText[6];  // "Lv255" plus terminator covers the uint8_t level.
    snprintf(levelText, sizeof(levelText), "Lv%u", static_cast<unsigned>(pet_.level()));
    const int16_t levelX = 128 - static_cast<int16_t>(strlen(levelText) * 5);
    display_.drawLine(3, 60, levelX - 6, 60);
    display_.drawSmallText(levelX, 63, levelText);
}

void UiController::renderMainMenu() {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(52, 12, "MENU");
    display_.drawLine(6, 16, 121, 16);
    for (uint8_t i = 0; i < kMenuItemCount; ++i) {
        const int16_t baseline = 23 + (i * 8);
        if (i == menuIndex_) display_.drawSmallText(8, baseline, ">");
        display_.drawSmallText(20, baseline, kMenuItems[i]);
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
    display_.drawStatusText(kStatusLabelX, 35, "等級");
    drawStatusNumber(display_, 35, static_cast<unsigned>(pet_.level()));

    display_.drawStatusText(kStatusLabelX, 53, "健康");
    drawStatusTextValue(display_, 53, pet_.isSick() ? "生病" : "正常");
}

void UiController::renderPlaceholder(const char* title) {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(43, 15, title);
    display_.drawLine(6, 20, 121, 20);
    display_.drawText(17, 40, "Not implemented");
    display_.drawText(31, 58, "Hold: Back");
}

void UiController::renderCare(const char* title, const char* stat, unsigned value) {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(40, 14, title);
    display_.drawLine(6, 19, 121, 19);
    char line[24];
    if (screen_ == ScreenId::TreatCare) {
        snprintf(line, sizeof(line), "Health: %s", pet_.isSick() ? "Sick" : "Good");
    } else {
        snprintf(line, sizeof(line), "%s: %u", stat, value);
    }
    display_.drawText(12, 35, line);
    display_.drawText(12, 49, screen_ == ScreenId::FeedCare ? "Press: +20" :
                      screen_ == ScreenId::CleanCare ? "Press: +30" : "Press: Treat");
    display_.drawSmallText(12, 60, "Hold: Back");
}

void UiController::renderDeathAnimation() {
    display_.clear();

    uint8_t dissolveStage = 0;
    if (deathAnimationElapsedMs_ >= 700) dissolveStage = 1;
    if (deathAnimationElapsedMs_ >= 1400) dissolveStage = 2;
    if (deathAnimationElapsedMs_ >= 2100) dissolveStage = 3;
    PetIcons::drawPetPlaceholderDissolve(display_, 42, 27, dissolveStage);

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
