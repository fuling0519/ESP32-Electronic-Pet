#pragma once

#include <stdint.h>

#include "hardware/Input.h"

namespace Hardware {
class Display;
class Sound;
}

namespace Pet {
class PetData;
enum class LifeStage : uint8_t;
}

namespace Ui {

enum class ScreenId {
    Boot,
    Home,
    MainMenu,
    FeedCare,
    CleanCare,
    TreatCare,
    PlayCare,
    RestPlaceholder,
    DetailedStatus,
    HatchTransition,
    GrowTransition,
    DeathAnimation,
    DeathMemorial,
};

enum class UiAction : uint8_t { None, Feed, Clean, Treat, Play };

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
    UiAction takeAction();

private:
    void setScreen(ScreenId screen);
    void renderBoot();
    void renderHome();
    void renderMainMenu();
    void renderDetailedStatus();
    void renderDetailedStatusPage1();
    void renderDetailedStatusPage2();
    void renderPlaceholder(const char* title);
    void renderCare(const char* title, const char* stat, unsigned value);
    void beginGrowthTransition(ScreenId screen, uint32_t now);
    void renderHatchTransition();
    void renderGrowTransition();
    void beginDeathAnimation(uint32_t now);
    void renderDeathAnimation();
    void renderDeathMemorial();
    uint8_t eggCrackStage(uint32_t now) const;

    Hardware::Display& display_;
    Hardware::Sound& sound_;
    const Pet::PetData& pet_;
    ScreenId screen_ = ScreenId::Boot;
    HomeFocus homeFocus_ = HomeFocus::None;
    uint8_t menuIndex_ = 0;
    uint8_t statusPage_ = 0;
    uint32_t bootStartedAt_ = 0;
    uint32_t growthTransitionStartedAt_ = 0;
    uint32_t growthTransitionElapsedMs_ = 0;
    uint8_t growthTransitionFrame_ = 0;
    uint32_t deathStartedAt_ = 0;
    uint32_t deathAnimationElapsedMs_ = 0;
    uint8_t deathAnimationFrame_ = 0;
    uint8_t homeAnimationFrame_ = 0;
    uint8_t eggCrackStage_ = 0;
    uint64_t eggAgeAtInitMilliseconds_ = 0;
    uint32_t eggAgeInitAtMs_ = 0;
    Pet::LifeStage observedLifeStage_;
    bool dirty_ = true;
    uint32_t lastRenderedPetRevision_ = 0;
    UiAction pendingAction_ = UiAction::None;
};

}  // namespace Ui
