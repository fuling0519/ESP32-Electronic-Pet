#pragma once

#include <stdint.h>

#include "hardware/Input.h"
#include "games/RpsGame.h"

namespace Hardware {
class Display;
class Sound;
}

namespace Pet {
class PetData;
enum class LifeStage : uint8_t;
}

namespace Storage { class Memorials; }

namespace Ui {

enum class ScreenId {
    Boot,
    Home,
    MainMenu,
    FeedCare,
    CleanCare,
    PlayCare,
    Rest,
    Sleeping,
    DetailedStatus,
    HatchTransition,
    GrowTransition,
    DeathAnimation,
    DeathMemorial,
    DeathOptions,
    Graveyard,
    DeleteMemorialConfirm,
    AdoptionBlocked,
};

enum class UiAction : uint8_t {
    None,
    Feed,
    Clean,
    Treat,
    FinishGame,
    StartNormalSleep,
    StartDeepSleep,
    Wake,
    AdoptNewEgg,
    DeleteMemorial,
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
                 const Pet::PetData& pet,
                 const Storage::Memorials& memorials,
                 Games::RpsGame::RandomSource random);

    void init(uint32_t now);
    // Returns true when visible UI state has changed.
    bool update(Hardware::InputEvent event, uint32_t now);
    void render();

    ScreenId screen() const;
    const char* screenName() const;
    uint8_t menuIndex() const;
    const char* selectedMenuItem() const;
    UiAction takeAction();
    uint8_t takeGameReward();
    void onGameRewardApplied(uint8_t actualGain, uint16_t expGain = 0,
                             uint8_t previousLevel = 0);
    uint8_t selectedMemorialIndex() const;
    void setMemorialReady(bool ready);
    void onSleepStarted(uint32_t now);
    void onFeedSucceeded(uint32_t now);
    void onCleanSucceeded(uint32_t now);
    void onTreatResult(bool success, uint32_t now);
    void onCareRewardApplied(uint8_t previousLevel, uint32_t now);
    void onWakeSucceeded();
    void onDeepSleepPending(bool pending, bool failed = false);
    void onAdoptionSucceeded(uint32_t now);
    void onMemorialDeleteResult(bool success);

private:
    void setScreen(ScreenId screen);
    void renderBoot();
    void renderHome();
    void renderPetFooter();
    void renderMainMenu();
    void renderDetailedStatus();
    void renderDetailedStatusPage1();
    void renderDetailedStatusPage2();
    void renderPlaceholder(const char* title);
    void renderCare(const char* title, const char* stat, unsigned value);
    void renderGame();
    void updateGame(Hardware::InputEvent event, uint32_t now);
    void renderRest();
    void renderSleeping();
    void beginGrowthTransition(ScreenId screen, uint32_t now);
    void renderHatchTransition();
    void renderGrowTransition();
    void beginDeathAnimation(uint32_t now);
    void renderDeathAnimation();
    void renderDeathMemorial();
    void renderDeathOptions();
    void renderGraveyard();
    void renderDeleteMemorialConfirm();
    void renderAdoptionBlocked();
    uint8_t eggCrackStage(uint32_t now) const;

    Hardware::Display& display_;
    Hardware::Sound& sound_;
    const Pet::PetData& pet_;
    const Storage::Memorials& memorials_;
    Games::RpsGame game_;
    uint8_t gameMoodGain_ = 0;
    uint16_t gameExpGain_ = 0;
    uint8_t gamePreviousLevel_ = 0, gameFinalLevel_ = 0;
    bool levelUpActive_ = false;
    uint32_t levelUpStartedAt_ = 0;
    uint8_t levelUpFrame_ = 0;
    uint8_t levelUpPreviousLevel_ = 0, levelUpFinalLevel_ = 0;
    bool careLevelUpPending_ = false;
    uint32_t careRewardAt_ = 0;
    void renderLevelUp();
    ScreenId screen_ = ScreenId::Boot;
    HomeFocus homeFocus_ = HomeFocus::None;
    uint8_t menuIndex_ = 0;
    bool treatmentNotice_ = false;
    uint32_t treatmentNoticeStartedAt_ = 0;
    void showTreatmentNotice(uint32_t now);
    uint8_t statusPage_ = 0;
    uint8_t restOption_ = 0;
    bool deepSleepPending_ = false;
    bool deepSleepFailed_ = false;
    uint8_t deathOptionIndex_ = 0;
    uint8_t memorialIndex_ = 0;
    uint8_t deleteConfirmIndex_ = 0;
    uint32_t bootStartedAt_ = 0;
    uint32_t growthTransitionStartedAt_ = 0;
    uint32_t growthTransitionElapsedMs_ = 0;
    uint8_t growthTransitionFrame_ = 0;
    uint32_t deathStartedAt_ = 0;
    uint32_t deathAnimationElapsedMs_ = 0;
    uint8_t deathAnimationFrame_ = 0;
    uint8_t homeAnimationFrame_ = 0;
    bool cleaning_ = false;
    uint32_t cleaningStartedAt_ = 0;
    uint8_t cleaningFrame_ = 0;
    bool treating_ = false;
    bool treatmentActionQueued_ = false;
    bool treatmentSucceeded_ = false;
    uint32_t treatmentStartedAt_ = 0;
    uint32_t treatmentElapsedMs_ = 0;
    Pet::LifeStage treatmentVisualStage_;
    void beginTreatment(uint32_t now);
    bool feeding_ = false;
    uint32_t feedingStartedAt_ = 0;
    uint8_t feedingFrame_ = 0;
    uint8_t feedingBowlStage_ = 0;
    uint32_t sleepStartedAtMs_ = 0;
    uint8_t sleepAnimationFrame_ = 0;
    uint8_t sleepZPhase_ = 0;
    Pet::LifeStage sleepVisualStage_;
    uint8_t eggCrackStage_ = 0;
    uint64_t eggAgeAtInitMilliseconds_ = 0;
    uint32_t eggAgeInitAtMs_ = 0;
    Pet::LifeStage observedLifeStage_;
    bool dirty_ = true;
    uint32_t lastRenderedPetRevision_ = 0;
    UiAction pendingAction_ = UiAction::None;
    bool memorialReady_ = false;
};

}  // namespace Ui
