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
constexpr uint8_t kMenuItemCount = 4;
constexpr bool kDebugUiEvents = true;
const char* const kMenuItems[kMenuItemCount] = {"Feed", "Play", "Rest", "Status"};

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
    bootStartedAt_ = now;
    dirty_ = true;
}

bool UiController::update(Hardware::InputEvent event, uint32_t now) {
    const ScreenId previousScreen = screen_;
    const HomeFocus previousHomeFocus = homeFocus_;
    const uint8_t previousMenuIndex = menuIndex_;

    if (kDebugUiEvents && event != Hardware::InputEvent::None) {
        Serial.print("UI received: ");
        Serial.println(inputEventName(event));
        Serial.print("UI screen: ");
        Serial.println(screenName());
    }

    if (screen_ == ScreenId::Boot) {
        if (now - bootStartedAt_ >= kBootDurationMs) setScreen(ScreenId::Home);
        return screen_ != previousScreen;
    }

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
                setScreen(static_cast<ScreenId>(static_cast<uint8_t>(ScreenId::FeedPlaceholder) + menuIndex_));
            } else if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::Home);
            }
            break;

        case ScreenId::FeedPlaceholder:
        case ScreenId::PlayPlaceholder:
        case ScreenId::RestPlaceholder:
        case ScreenId::StatusPlaceholder:
            if (event == Hardware::InputEvent::LongPress) {
                sound_.playCancel();
                setScreen(ScreenId::MainMenu);
            }
            break;

        case ScreenId::Boot:
            break;
    }

    if (kDebugUiEvents && menuIndex_ != previousMenuIndex) {
        Serial.printf("Menu index: %u -> %u\n", previousMenuIndex, menuIndex_);
    }
    return screen_ != previousScreen || homeFocus_ != previousHomeFocus ||
           menuIndex_ != previousMenuIndex;
}

void UiController::render() {
    if (!dirty_ || !display_.isInitialized()) return;

    switch (screen_) {
        case ScreenId::Boot: renderBoot(); break;
        case ScreenId::Home: renderHome(); break;
        case ScreenId::MainMenu: renderMainMenu(); break;
        case ScreenId::FeedPlaceholder: renderPlaceholder("FEED"); break;
        case ScreenId::PlayPlaceholder: renderPlaceholder("PLAY"); break;
        case ScreenId::RestPlaceholder: renderPlaceholder("REST"); break;
        case ScreenId::StatusPlaceholder: renderPlaceholder("STATUS"); break;
    }
    display_.update();
    dirty_ = false;
}

ScreenId UiController::screen() const { return screen_; }

const char* UiController::screenName() const {
    switch (screen_) {
        case ScreenId::Boot: return "BOOT";
        case ScreenId::Home: return "HOME";
        case ScreenId::MainMenu: return "MAIN_MENU";
        case ScreenId::FeedPlaceholder: return "FEED_PLACEHOLDER";
        case ScreenId::PlayPlaceholder: return "PLAY_PLACEHOLDER";
        case ScreenId::RestPlaceholder: return "REST_PLACEHOLDER";
        case ScreenId::StatusPlaceholder: return "STATUS_PLACEHOLDER";
    }
    return "UNKNOWN";
}

uint8_t UiController::menuIndex() const { return menuIndex_; }
const char* UiController::selectedMenuItem() const { return kMenuItems[menuIndex_]; }

void UiController::setScreen(ScreenId screen) {
    if (screen_ == screen) return;
    screen_ = screen;
    if (screen_ == ScreenId::Home) homeFocus_ = HomeFocus::None;
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
        const int16_t baseline = 29 + (i * 11);
        if (i == menuIndex_) display_.drawText(8, baseline, ">");
        display_.drawText(24, baseline, kMenuItems[i]);
    }
}

void UiController::renderPlaceholder(const char* title) {
    display_.clear();
    display_.drawFrame(0, 0, 128, 64);
    display_.drawText(43, 15, title);
    display_.drawLine(6, 20, 121, 20);
    display_.drawText(17, 40, "Not implemented");
    display_.drawText(31, 58, "Hold: Back");
}

}  // namespace Ui
