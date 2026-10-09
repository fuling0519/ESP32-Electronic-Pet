#include <Arduino.h>
#include <esp_system.h>
#include <limits.h>

#include "hardware/Display.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
#include "pet/PetName.h"
#include "pet/PetClock.h"
#include "storage/Save.h"
#include "storage/Adoption.h"
#include "storage/DeviceSettings.h"
#include "storage/Farewell.h"
#include "storage/Memorials.h"
#if defined(PET_MEMORIAL_SAMPLE_MODE)
#include "storage/MemorialSample.h"
#endif
#include "ui/UiController.h"
#include "ble/BleLink.h"
#include "power/DeepSleep.h"
#include "HardwareConfig.h"
#if defined(PET_SAD_TEST_MODE)
#include "pet/SadQuickTest.h"
#endif

namespace {
constexpr bool kDebugUi = true;

Pet::SpeciesId newEggSpecies(uint32_t bits) {
#if defined(PET_WYVERN_TEST_MODE)
    (void)bits;
    return Pet::SpeciesId::Wyvern;
#elif defined(PET_SAD_TEST_MODE) || defined(PET_TREATMENT_TEST_MODE)
    (void)bits;
    return Pet::SpeciesId::Bird;
#else
    return Pet::speciesForNewEgg(bits);
#endif
}

// Application owns the current pet. UI borrows it read-only, without a copy.
class Application {
public:
    void setup();
    void loop();

private:
    void printUiState(Hardware::InputEvent event);
    bool prepareNewEgg(Pet::PetData& target, uint64_t petId);
    void generateNewName(char (&name)[Pet::kPetNameMaxLength + 1]);
    bool ensureCurrentMemorialSaved();
    bool startNormalSleep(uint32_t now);
    bool wakeFromNormalSleep(uint32_t now);
    bool adoptNewEgg(uint32_t now);
    bool deleteSelectedMemorial();
    void requestDeepSleep(uint32_t now);
    void serviceDeepSleep(Hardware::InputEvent event, uint32_t now);

    Hardware::Display display;
    Hardware::Input input;
    Hardware::Sound sound;
    Storage::Save save;
    Storage::Adoption adoption;
    Storage::DeviceSettings settings;
    Pet::PetData pet;
    Pet::PetClock petClock;
    Storage::Memorials memorials;
    Ble::Link ble;
    Ui::UiController ui{display, sound, pet, memorials, esp_random};
    uint64_t lastEggCheckpointMinute = 0;
    bool memorialReady = false;
    uint32_t farewellRetryAt = 0;
    uint32_t lastCapacityCheckAt = 0;
    bool appReady = false;
    bool deepPending = false;
    uint32_t deepRequestedAt = 0;
    uint32_t releaseStartedAt = 0;
    bool releaseObserved = false;
    uint32_t resumedRemainderMs = 0;
    bool resumedFromDeep = false;
#if defined(PET_SAD_TEST_MODE)
    uint32_t sadTestStartedAt = 0;
    bool sadTestAutoPending = false;
    void serviceSadTest(uint32_t now);
#endif
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

void Application::generateNewName(char (&name)[Pet::kPetNameMaxLength + 1]) {
    for (uint8_t attempt = 0; attempt < 8; ++attempt) {
        Pet::generateAbbName(esp_random(), name);
        if (!memorials.containsName(name)) break;
    }
}

bool Application::prepareNewEgg(Pet::PetData& target, uint64_t petId) {
    char name[Pet::kPetNameMaxLength + 1]{};
    generateNewName(name);
    return target.startNewEgg(petId, name, newEggSpecies(esp_random()));
}

bool Application::ensureCurrentMemorialSaved() {
    return Storage::finishFarewell(pet, memorials, save);
}

bool Application::startNormalSleep(uint32_t now) {
    const Pet::PetData before = pet;
#if defined(PET_SLEEP_TEST_MODE)
    pet.setSatiety(0);
#endif
    if (!pet.beginNormalSleep()) {
        pet = before;
        return false;
    }
    if (!save.save(pet)) {
        pet = before;
        return false;
    }
    petClock.reset(now);
    ui.onSleepStarted(now);
    return true;
}

bool Application::wakeFromNormalSleep(uint32_t now) {
    const Pet::PetData before = pet;
    if (!pet.wake()) return false;
    if (!save.save(pet)) {
        pet = before;
        return false;
    }
    petClock.reset(now);
    ui.onWakeSucceeded();
    return true;
}

bool Application::adoptNewEgg(uint32_t now) {
    if (!pet.isEnded() || !memorialReady ||
        memorials.count() > Storage::kMemorialLimit) return false;
    uint64_t highestId = memorials.highestPetId();
    if (pet.petId() > highestId) highestId = pet.petId();
    if (highestId == UINT64_MAX) return false;
    if (!adoption.prepared()) {
        char name[Pet::kPetNameMaxLength + 1]{};
        generateNewName(name);
        const uint32_t bits = newEggSpecies(esp_random()) == Pet::SpeciesId::Wyvern ? 1 : 0;
        if (!adoption.prepare(highestId + 1, name, bits)) return false;
    }
    if (!adoption.commit(save, pet)) return false;
    memorialReady = false;
    ui.setMemorialReady(false);
    petClock.reset(now);
    lastEggCheckpointMinute = 0;
    ui.onAdoptionSucceeded(now);
    return true;
}

bool Application::deleteSelectedMemorial() {
    const uint8_t index = ui.selectedMemorialIndex();
    const Storage::MemorialRecord* selected = memorials.at(index);
    if (selected == nullptr ||
        (pet.isEnded() && selected->petId == pet.petId())) return false;
    return save.removeMemorial(index, memorials);
}

void Application::setup() {
    Power::releaseWakePins();
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Electronic Pet Boot ===");
    Serial.printf("Reset reason: %d\n", static_cast<int>(esp_reset_reason()));
    if (!display.init()) {
        Serial.println("Display init failed.");
        return;
    }
    int retainedX = -1, retainedY = -1;
    Power::retainedCalibration(retainedX, retainedY);
    input.init(retainedX, retainedY);
    sound.init();
    if (!settings.init()) Serial.println("Device settings unavailable; sound enabled by default.");
    sound.setVolume(settings.volume());
    if (!save.init()) {
        Serial.println("NVS init failed; this session will not be saved.");
    } else {
        switch (save.loadMemorials(memorials)) {
            case Storage::LoadStatus::Loaded:
                Serial.printf("Loaded %u memorial(s).\n", memorials.count());
                break;
            case Storage::LoadStatus::Empty:
                Serial.println("No memorial archive yet.");
                break;
            case Storage::LoadStatus::Invalid:
                Serial.println("Memorial archive is invalid; archive writes blocked.");
                break;
            case Storage::LoadStatus::Unavailable:
                Serial.println("Memorial archive unavailable.");
                break;
        }
#if defined(PET_DEATH_TEST_MODE) || defined(PET_SLEEP_TEST_MODE) || defined(PET_TREATMENT_TEST_MODE) || defined(PET_SAD_TEST_MODE)
        const bool ignoreTestSave = esp_reset_reason() != ESP_RST_DEEPSLEEP;
#else
        const bool ignoreTestSave = false;
#endif
        if (ignoreTestSave) {
            Serial.println("Quick test mode: stored test state ignored on cold startup.");
        } else {
        switch (save.load(pet)) {
            case Storage::LoadStatus::Loaded:
                Serial.println("Pet save loaded.");
                break;
            case Storage::LoadStatus::Empty:
                if (!prepareNewEgg(pet, memorials.highestPetId() + 1)) {
                    Serial.println("Failed to create initial pet identity.");
                } else {
#if defined(PET_DEEP_SLEEP_TEST_MODE)
                    pet.advanceSeconds(Pet::PetData::kAdultAgeSeconds);
                    pet.setSatiety(80); pet.setCleanliness(100); pet.setMood(50);
#endif
                    if (save.save(pet)) Serial.println("Created initial pet save.");
                    else Serial.println("Initial pet save failed.");
                }
                break;
            case Storage::LoadStatus::Invalid:
                Serial.println("Pet saves are invalid; writes blocked to preserve recovery data.");
                break;
            case Storage::LoadStatus::Unavailable:
                Serial.println("Pet save unavailable.");
                break;
        }
#if defined(PET_GROWTH_TEST_MODE)
        if (esp_reset_reason() != ESP_RST_DEEPSLEEP && pet.lifeStage() == Pet::LifeStage::Adult) {
            pet.startNewEgg();
            if (save.save(pet)) Serial.println("Growth test restarted from egg.");
        }
#endif
        }
    }
    resumedFromDeep = Power::resume(pet, resumedRemainderMs);
    if (resumedFromDeep && !save.save(pet)) {
        Serial.println("Failed to save deep-sleep settlement; retry scheduled.");
        save.markDirty(millis());
    }
    if (pet.sleepMode() == Pet::SleepMode::Normal || pet.sleepMode() == Pet::SleepMode::Deep) {
        if (pet.wake()) {
            if (save.save(pet)) {
                Serial.println("Interrupted normal sleep ended at startup.");
            } else {
                Serial.println("Failed to save startup wake state.");
            }
        }
    }
    if (pet.isEnded()) memorialReady = ensureCurrentMemorialSaved();
#if defined(PET_MEMORIAL_SAMPLE_MODE)
    Serial.println(Storage::ensureMemorialSample(pet, memorials, save) ?
        "TestRIP memorial sample ready." : "Could not save TestRIP memorial sample.");
#endif
    ui.setMemorialReady(memorialReady);
    const uint32_t now = millis();
    petClock.reset(now - resumedRemainderMs);
#if defined(PET_SAD_TEST_MODE)
    if (!resumedFromDeep) {
        if (!prepareNewEgg(pet, memorials.highestPetId() + 1)) {
            Serial.println("Failed to create sadness test pet.");
            return;
        }
#if defined(PET_SAD_TEST_BABY)
        pet.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
#else
        pet.advanceSeconds(Pet::PetData::kAdultAgeSeconds);
#endif
        Pet::applySadTestPreset(pet, '0');
        if (!save.save(pet)) Serial.println("Failed to save initial sadness test state.");
    }
    sadTestStartedAt = now;
    sadTestAutoPending = !resumedFromDeep;
    Serial.println("Sad test: normal start, low mood after 8s on Home; cooldown=10s.");
    Serial.println("Commands: 0 normal, 1 mood40, 2 satiety25, 3 clean25, 4 sick, 5 multiple, 6 mood44, 7 mood45, 8 needs29, 9 needs30.");
#elif defined(PET_TREATMENT_TEST_MODE)
    if (!resumedFromDeep) {
        if (!prepareNewEgg(pet, memorials.highestPetId() + 1)) {
            Serial.println("Failed to create treatment test pet.");
            return;
        }
#if defined(PET_TREATMENT_TEST_BABY)
        pet.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
#else
        pet.advanceSeconds(Pet::PetData::kAdultAgeSeconds);
#endif
        pet.setSatiety(60);
        pet.setMood(55);
        pet.setCleanliness(0);
        pet.setSick(true);
        if (!save.save(pet)) Serial.println("Failed to save initial treatment test state.");
    }
    Serial.println("Treatment test: starts sick; reset to repeat. Normal death timing.");
#elif defined(PET_DEATH_TEST_MODE)
    if (!resumedFromDeep) pet.setSick(true);
    Serial.println("Death test mode: pet starts sick and dies after 30 seconds.");
#elif defined(PET_SLEEP_TEST_MODE)
    if (!resumedFromDeep) {
    pet.setMood(50);
    if (!save.save(pet)) Serial.println("Failed to save initial sleep test state.");
    }
    Serial.println("Sleep test mode: entering sleep sets satiety to zero; mood recovers every 20 seconds with one low need (paused with two), sickness starts after 20 seconds, and death follows 30 seconds later.");
#endif
    ui.init(now);
    ui.setStorageStatus(save.hasSaveFailure(), save.capacityLow());
    lastCapacityCheckAt = now;
    if (resumedFromDeep && !pet.isEnded()) ui.onWakeSucceeded();
    lastEggCheckpointMinute = pet.lifeStage() == Pet::LifeStage::Egg ?
        pet.ageSeconds() / 60 : 0;
    appReady = true;
    if (!ble.init()) Serial.println("BLE init failed; pet remains available offline.");
    printUiState(Hardware::InputEvent::None);
    ui.render();
}

void Application::requestDeepSleep(uint32_t now) {
    if (pet.isEnded() || pet.lifeStage() == Pet::LifeStage::Egg ||
        pet.sleepMode() != Pet::SleepMode::Awake) { sound.playFailure(); return; }
    deepPending = true;
    deepRequestedAt = now;
    releaseObserved = false;
    ui.onDeepSleepPending(true);
}

void Application::serviceDeepSleep(Hardware::InputEvent event, uint32_t now) {
    if (!deepPending) return;
    if (event == Hardware::InputEvent::LongPress || pet.isEnded() ||
        now - deepRequestedAt >= HardwareConfig::Sleep::ReleaseTimeoutMs) {
        deepPending = false; ui.onDeepSleepPending(false); sound.playCancel(); return;
    }
    if (input.switchHeld()) { releaseObserved = false; return; }
    if (!releaseObserved) { releaseObserved = true; releaseStartedAt = now; return; }
    if (now - releaseStartedAt < HardwareConfig::Sleep::ReleaseStableMs) return;
    deepPending = false;
    const Pet::PetData before = pet;
    if (!Power::configure() || !pet.beginDeepSleep()) {
        Power::cancel(); ui.onDeepSleepPending(false, true); sound.playFailure(); return;
    }
    if (!save.save(pet)) {
        pet = before; Power::cancel();
        ui.onDeepSleepPending(false, true); sound.playFailure(); return;
    }
    // Recheck a level-triggered wake pin after the flash write. If pressed,
    // cancel cleanly rather than entering an immediate sleep/wake loop.
    if (Power::wakePressed()) {
        pet = before; Power::cancel();
        if (!save.save(pet)) save.markDirty(millis());
        ui.onDeepSleepPending(false); return;
    }
    Power::retain(pet, input.centerX(), input.centerY(), petClock.remainderMilliseconds(millis()));
    sound.stopTone();
    display.setPowerSave(true);
    ble.stop();
    Power::enter();
}

#if defined(PET_SAD_TEST_MODE)
void Application::serviceSadTest(uint32_t now) {
    while (Serial.available() > 0) {
        const char command = static_cast<char>(Serial.read());
        if (command < '0' || command > '9') continue;
        if (Pet::applySadTestPreset(pet, command)) {
            sadTestAutoPending = false;
            save.markDirty(now);
            Serial.printf("Sad test preset %c: mood=%u satiety=%u clean=%u sick=%u\n",
                command, pet.mood(), pet.satiety(), pet.cleanliness(), pet.isSick());
        } else Serial.println("Sad test preset ignored: requires a living awake bird.");
    }
    if (sadTestAutoPending && now - sadTestStartedAt >= 8000 &&
        ui.screen() == Ui::ScreenId::Home && !sound.isPlaying() &&
        Pet::applySadTestPreset(pet, '1')) {
        sadTestAutoPending = false;
        save.markDirty(now);
        Serial.println("Sad test automatic low mood: care reminder should sound once.");
    }
}
#endif

void Application::loop() {
    if (!appReady) return;

    const uint32_t now = millis();
#if defined(PET_SAD_TEST_MODE)
    serviceSadTest(now);
#endif
    const Pet::HealthState healthBeforeAdvance = pet.healthState();
    const Pet::LifeStage stageBeforeAdvance = pet.lifeStage();
    const uint64_t ageBeforeAdvanceMs = pet.ageSeconds() * 1000ULL;
    const uint32_t elapsedSeconds = petClock.consumeElapsedSeconds(now);
    if (pet.sleepMode() == Pet::SleepMode::Normal) {
        pet.advanceSleepSeconds(elapsedSeconds);
    } else {
        pet.advanceSeconds(elapsedSeconds);
    }
#if defined(PET_MEMORIAL_TEST_MODE)
    if (pet.lifeStage() != Pet::LifeStage::Egg &&
        pet.healthState() == Pet::HealthState::Healthy) {
        pet.setSick(true);
    }
#endif
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
        const bool deathSaved = save.save(pet);
        if (!deathSaved) Serial.println("Failed to save pet death state.");
        memorialReady = deathSaved && ensureCurrentMemorialSaved();
        ui.setMemorialReady(memorialReady);
        if (!memorialReady) Serial.println("Failed to save current memorial.");
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
    if (pet.isEnded() && !memorialReady && now - farewellRetryAt >= 10000) {
        farewellRetryAt = now;
        memorialReady = ensureCurrentMemorialSaved();
        ui.setMemorialReady(memorialReady);
    }
    const Hardware::InputEvent event = input.update();
    if (kDebugUi && event != Hardware::InputEvent::None) {
        Serial.print("Physical/Input event: ");
        Serial.println(inputEventName(event));
    }
    const bool changed = ui.update(event, now, input.directionNeutral());
    const uint32_t revisionBeforeAction = pet.displayRevision();
    const Pet::LifeStage stageBeforeAction = pet.lifeStage();
    const uint8_t levelBeforeAction = pet.level();
    const Ui::UiAction action = ui.takeAction();
    bool actionSavedImmediately = false;
    switch (action) {
        case Ui::UiAction::Feed:
            if (pet.feed()) {
                sound.playSuccess();
                ui.onFeedSucceeded(now);
            } else sound.playFailure();
            break;
        case Ui::UiAction::Clean:
            if (pet.clean()) {
                sound.playSuccess();
                ui.onCleanSucceeded(now);
            } else sound.playFailure();
            break;
        case Ui::UiAction::FinishGame: {
            const uint8_t reward = ui.takeGameReward();
            const uint8_t before = pet.mood();
            const uint8_t previousLevel = pet.level();
            uint16_t expGain = 0;
            if (reward && !pet.isEnded() && !pet.isSick() &&
                pet.lifeStage() != Pet::LifeStage::Egg &&
                pet.sleepMode() == Pet::SleepMode::Awake) {
                pet.changeMood(reward);
                expGain = pet.gainExp(ui.gameExpReward());
            }
            ui.onGameRewardApplied(pet.mood() - before, expGain, previousLevel);
            break;
        }
        case Ui::UiAction::Treat: {
            const bool success = pet.treat();
            ui.onTreatResult(success, now);
            if (success) sound.playSuccess();
            else sound.playFailure();
            break;
        }
        case Ui::UiAction::StartNormalSleep:
            if (startNormalSleep(now)) {
                sound.playConfirm();
                actionSavedImmediately = true;
            } else {
                ui.onNormalSleepFailed();
                sound.playFailure();
            }
            break;
        case Ui::UiAction::StartDeepSleep:
            requestDeepSleep(now);
            break;
        case Ui::UiAction::Wake:
            if (wakeFromNormalSleep(now)) {
                sound.playSuccess();
                actionSavedImmediately = true;
            } else {
                sound.playFailure();
            }
            break;
        case Ui::UiAction::SendOff:
            memorialReady = Storage::beginFarewell(pet, memorials, save);
            farewellRetryAt = now;
            ui.setMemorialReady(memorialReady);
            ui.onFarewellResult(memorialReady, now);
            actionSavedImmediately = pet.isDeparted();
            if (!memorialReady) sound.playFailure();
            break;
        case Ui::UiAction::RetryFarewell:
            memorialReady = ensureCurrentMemorialSaved();
            farewellRetryAt = now;
            ui.setMemorialReady(memorialReady);
            ui.onFarewellResult(memorialReady, now);
            break;
        case Ui::UiAction::AdoptNewEgg:
            if (adoptNewEgg(now)) sound.playSuccess();
            else sound.playFailure();
            break;
        case Ui::UiAction::DeleteMemorial:
            ui.onMemorialDeleteResult(deleteSelectedMemorial());
            break;
        case Ui::UiAction::None: break;
        case Ui::UiAction::SaveVolume:
            ui.onVolumeSaveResult(settings.saveVolume(sound.volume()));
            break;
    }
    if (action == Ui::UiAction::Feed || action == Ui::UiAction::Clean ||
        action == Ui::UiAction::Treat) ui.onCareRewardApplied(levelBeforeAction, now);
    if (actionSavedImmediately) {
        // Entering and leaving sleep already wrote a verified checkpoint.
    } else if (pet.lifeStage() != stageBeforeAction) {
        if (!save.save(pet)) Serial.println("Failed to save pet growth stage.");
    } else if (pet.displayRevision() != revisionBeforeAction) {
        save.markDirty(now);
    }
    save.update(pet, now);
    if (now - lastCapacityCheckAt >= 30000) {
        lastCapacityCheckAt = now;
        save.refreshCapacity();
    }
    ui.setStorageStatus(save.hasSaveFailure(), save.capacityLow());
    if (changed) printUiState(event);
    sound.update();
    ui.render();
    ble.update(pet, now);
    serviceDeepSleep(event, now);
}

Application application;
}  // namespace

void setup() { application.setup(); }
void loop() { application.loop(); }
