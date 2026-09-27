#include <Arduino.h>

#include "hardware/Display.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
#include "pet/PetClock.h"
#include "storage/Save.h"
#include "ui/UiController.h"

namespace {
constexpr bool kDebugUi = true;

// Application owns the current pet. UI borrows it read-only, without a copy.
class Application {
public:
    void setup();
    void loop();

private:
    void printUiState(Hardware::InputEvent event);

    Hardware::Display display;
    Hardware::Input input;
    Hardware::Sound sound;
    Storage::Save save;
    Pet::PetData pet;
    Pet::PetClock petClock;
    Ui::UiController ui{display, sound, pet};
    uint64_t lastEggCheckpointMinute = 0;
    bool appReady = false;
};

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

void Application::printUiState(Hardware::InputEvent event) {
    if (!kDebugUi) return;
    (void)event;
    Serial.print("Screen: ");
    Serial.println(ui.screenName());
    if (ui.screen() == Ui::ScreenId::MainMenu) {
        Serial.printf("Menu index: %u\nSelected: %s\n", ui.menuIndex(), ui.selectedMenuItem());
    }
}

void Application::setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Phase 1A UI Navigation Test ===");
    if (!display.init()) {
        Serial.println("Display init failed.");
        return;
    }
    input.init();
    sound.init();
    if (!save.init()) {
        Serial.println("NVS init failed; this session will not be saved.");
    } else {
#if defined(PET_DEATH_TEST_MODE)
        Serial.println("Death test mode: stored test state is ignored on startup.");
#else
        switch (save.load(pet)) {
            case Storage::LoadStatus::Loaded:
                Serial.println("Pet save loaded.");
                break;
            case Storage::LoadStatus::Empty:
                pet.startNewEgg();
                if (save.save(pet)) Serial.println("Created initial pet save.");
                else Serial.println("Initial pet save failed.");
                break;
            case Storage::LoadStatus::Invalid:
                Serial.println("Pet saves are invalid; writes blocked to preserve recovery data.");
                break;
            case Storage::LoadStatus::Unavailable:
                Serial.println("Pet save unavailable.");
                break;
        }
#if defined(PET_GROWTH_TEST_MODE)
        if (pet.lifeStage() == Pet::LifeStage::Adult) {
            pet.startNewEgg();
            if (save.save(pet)) Serial.println("Growth test restarted from egg.");
        }
#endif
#endif
    }
    const uint32_t now = millis();
    petClock.reset(now);
#if defined(PET_DEATH_TEST_MODE)
    pet.setSick(true);
    Serial.println("Death test mode: pet starts sick and dies after 30 seconds.");
#endif
    ui.init(now);
    lastEggCheckpointMinute = pet.lifeStage() == Pet::LifeStage::Egg ?
        pet.ageSeconds() / 60 : 0;
    appReady = true;
    printUiState(Hardware::InputEvent::None);
    ui.render();
}

void Application::loop() {
    if (!appReady) return;

    const uint32_t now = millis();
    const Pet::HealthState healthBeforeAdvance = pet.healthState();
    const Pet::LifeStage stageBeforeAdvance = pet.lifeStage();
    const uint64_t ageBeforeAdvanceMs = pet.ageSeconds() * 1000ULL;
    pet.advanceSeconds(petClock.consumeElapsedSeconds(now));
    const uint64_t ageAfterAdvanceMs = pet.ageSeconds() * 1000ULL;
    const bool crossedEggCrackMilestone =
        stageBeforeAdvance == Pet::LifeStage::Egg &&
        pet.lifeStage() == Pet::LifeStage::Egg &&
        ((ageBeforeAdvanceMs < Pet::PetData::kEggSmallCrackAgeMilliseconds &&
          ageAfterAdvanceMs >= Pet::PetData::kEggSmallCrackAgeMilliseconds) ||
         (ageBeforeAdvanceMs < Pet::PetData::kEggLargeCrackAgeMilliseconds &&
          ageAfterAdvanceMs >= Pet::PetData::kEggLargeCrackAgeMilliseconds));
    const uint64_t eggAgeMinute = pet.lifeStage() == Pet::LifeStage::Egg ?
        pet.ageSeconds() / 60 : 0;
    const bool eggCheckpointDue =
        pet.lifeStage() == Pet::LifeStage::Egg &&
        eggAgeMinute > lastEggCheckpointMinute;
    if (pet.isDead() && healthBeforeAdvance != Pet::HealthState::Dead) {
        if (!save.save(pet)) Serial.println("Failed to save pet death state.");
    } else if (pet.lifeStage() != stageBeforeAdvance) {
        if (!save.save(pet)) Serial.println("Failed to save pet growth stage.");
    } else if (crossedEggCrackMilestone) {
        if (!save.save(pet)) Serial.println("Failed to save egg crack progress.");
    } else if (eggCheckpointDue) {
        if (!save.save(pet)) Serial.println("Failed to save egg age checkpoint.");
    } else if (pet.healthState() != healthBeforeAdvance) {
        save.markDirty(now);
    }
    if (eggCheckpointDue) lastEggCheckpointMinute = eggAgeMinute;
    const Hardware::InputEvent event = input.update();
    if (kDebugUi && event != Hardware::InputEvent::None) {
        Serial.print("Physical/Input event: ");
        Serial.println(inputEventName(event));
    }
    const bool changed = ui.update(event, now);
    const uint32_t revisionBeforeAction = pet.displayRevision();
    const Pet::LifeStage stageBeforeAction = pet.lifeStage();
    const Ui::UiAction action = ui.takeAction();
    switch (action) {
        case Ui::UiAction::Feed:
            if (pet.feed()) sound.playSuccess(); else sound.playFailure();
            break;
        case Ui::UiAction::Clean:
            if (pet.clean()) sound.playSuccess(); else sound.playFailure();
            break;
        case Ui::UiAction::Play:
            if (pet.play()) sound.playSuccess(); else sound.playFailure();
            break;
        case Ui::UiAction::Treat:
            if (pet.treat()) sound.playSuccess();
            else sound.playFailure();
            break;
        case Ui::UiAction::None: break;
    }
    if (pet.lifeStage() != stageBeforeAction) {
        if (!save.save(pet)) Serial.println("Failed to save pet growth stage.");
    } else if (pet.displayRevision() != revisionBeforeAction) {
        save.markDirty(now);
    }
    save.update(pet, now);
    if (changed) printUiState(event);
    sound.update();
    ui.render();
}

Application application;
}  // namespace

void setup() { application.setup(); }
void loop() { application.loop(); }
