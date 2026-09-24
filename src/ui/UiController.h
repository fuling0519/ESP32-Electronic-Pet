#pragma once

#include <stdint.h>

#include "hardware/Input.h"

namespace Hardware {
class Display;
class Sound;
}

namespace Pet {
class PetData;
}

namespace Ui {

enum class ScreenId {
    Boot,
    Home,
    MainMenu,
    FeedPlaceholder,
    PlayPlaceholder,
    RestPlaceholder,
    DetailedStatus,
};

// Home currently has one selectable shortcut. Keep this separate from screen
// state so future shortcuts can extend the enum without a navigation framework.
enum class HomeFocus {
    None,
    Status,
};

class UiController {
public:
    UiController(Hardware::Display& display, Hardware::Sound& sound,
                 const Pet::PetData& pet);

    void init(uint32_t now);
    // Returns true when visible UI state has changed.
    bool update(Hardware::InputEvent event, uint32_t now);
    void render();

    ScreenId screen() const;
    const char* screenName() const;
    uint8_t menuIndex() const;
    const char* selectedMenuItem() const;

private:
    void setScreen(ScreenId screen);
    void renderBoot();
    void renderHome();
    void renderMainMenu();
    void renderDetailedStatus();
    void renderDetailedStatusPage1();
    void renderDetailedStatusPage2();
    void renderPlaceholder(const char* title);

    Hardware::Display& display_;
    Hardware::Sound& sound_;
    const Pet::PetData& pet_;
    ScreenId screen_ = ScreenId::Boot;
    HomeFocus homeFocus_ = HomeFocus::None;
    uint8_t menuIndex_ = 0;
    uint8_t statusPage_ = 0;
    uint32_t bootStartedAt_ = 0;
    bool dirty_ = true;
};

}  // namespace Ui
