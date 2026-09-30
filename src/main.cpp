#include <Arduino.h>
#include <esp_system.h>
#include <limits.h>

#include "hardware/Display.h"
#include "hardware/DeviceIdentity.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
#include "pet/PetName.h"
#include "pet/PetClock.h"
#include "storage/Save.h"
#include "storage/Memorials.h"
#include "ui/UiController.h"
#include "ble/BleLink.h"

namespace {
constexpr bool kDebugUi = true;

// Application owns the current pet. UI borrows it read-only, without a copy.
class Application {
public:
    void setup();
    void loop();

private:
    void printUiState(Hardware::InputEvent event);
    bool prepareNewEgg(Pet::PetData& target, uint64_t petId);
    bool ensureCurrentMemorialSaved();
    bool startNormalSleep(uint32_t now);
    bool wakeFromNormalSleep(uint32_t now);
    bool adoptNewEgg(uint32_t now);
    bool deleteSelectedMemorial();

    Hardware::Display display;
    Hardware::Input input;
    Hardware::Sound sound;
    Storage::Save save;
    Pet::PetData pet;
    Pet::PetClock petClock;
    Storage::Memorials memorials;
    Ble::Link ble;
    Ui::UiController ui{display, sound, pet, memorials};
    uint64_t lastEggCheckpointMinute = 0;
    bool memorialReady = false;
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

bool Application::prepareNewEgg(Pet::PetData& target, uint64_t petId) {
    char name[Pet::kPetNameMaxLength + 1]{};
    for (uint8_t attempt = 0; attempt < 8; ++attempt) {
        Pet::generateAbbName(esp_random(), name);
        if (!memorials.containsName(name)) break;
    }
    return target.startNewEgg(petId, name);
}

bool Application::ensureCurrentMemorialSaved() {
    if (!pet.isDead()) return false;
    if (memorials.containsPet(pet.petId())) return true;
    Storage::Memorials updated = memorials;
    if (!updated.append(pet) || !save.saveMemorials(updated)) return false;
    memorials = updated;
    return true;
}

bool Application::startNormalSleep(uint32_t now) {
    const Pet::PetSnapshotV1 before = pet.snapshot();
#if defined(PET_SLEEP_TEST_MODE)
    pet.setSatiety(0);
#endif
    if (!pet.beginNormalSleep()) {
        pet.restore(before);
        return false;
    }
    if (!save.save(pet)) {
        pet.restore(before);
        return false;
    }
    petClock.reset(now);
    ui.onSleepStarted(now);
    return true;
}

bool Application::wakeFromNormalSleep(uint32_t now) {
    const Pet::PetSnapshotV1 before = pet.snapshot();
    if (!pet.wake()) return false;
    if (!save.save(pet)) {
        pet.restore(before);
        return false;
    }
    petClock.reset(now);
    ui.onWakeSucceeded();
    return true;
}

bool Application::adoptNewEgg(uint32_t now) {
    if (!pet.isDead() || !memorialReady ||
        memorials.count() > Storage::kMemorialLimit) return false;
    uint64_t highestId = memorials.highestPetId();
    if (pet.petId() > highestId) highestId = pet.petId();
    if (highestId == UINT64_MAX) return false;
    Pet::PetData candidate;
    if (!prepareNewEgg(candidate, highestId + 1) || !save.save(candidate) ||
        !pet.restore(candidate.snapshot())) return false;
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
        (pet.isDead() && selected->petId == pet.petId())) return false;
    Storage::Memorials updated = memorials;
    if (!updated.remove(index) || !save.saveMemorials(updated)) return false;
    memorials = updated;
    return true;
}

void Application::setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Electronic Pet Boot ===");
    const char* deviceId = Hardware::deviceId();
    Serial.printf("Device ID: %s\n", deviceId[0] ? deviceId : "unavailable");
    if (!display.init()) {
        Serial.println("Display init failed.");
        return;
    }
    input.init();
    sound.init();
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
#if defined(PET_DEATH_TEST_MODE) || defined(PET_SLEEP_TEST_MODE)
#if defined(PET_SLEEP_TEST_MODE)
        Serial.println("Sleep test mode: stored test state is ignored on startup.");
#else
        Serial.println("Death test mode: stored test state is ignored on startup.");
#endif
#else
        switch (save.load(pet)) {
            case Storage::LoadStatus::Loaded:
                Serial.println("Pet save loaded.");
                break;
            case Storage::LoadStatus::Empty:
                if (!prepareNewEgg(pet, memorials.highestPetId() + 1)) {
                    Serial.println("Failed to create initial pet identity.");
                } else if (save.save(pet)) Serial.println("Created initial pet save.");
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
    if (pet.sleepMode() == Pet::SleepMode::Normal) {
        if (pet.wake()) {
            if (save.save(pet)) {
                Serial.println("Interrupted normal sleep ended at startup.");
            } else {
                Serial.println("Failed to save startup wake state.");
            }
        }
    }
    if (pet.isDead()) memorialReady = ensureCurrentMemorialSaved();
    ui.setMemorialReady(memorialReady);
    const uint32_t now = millis();
    petClock.reset(now);
#if defined(PET_DEATH_TEST_MODE)
    pet.setSick(true);
    Serial.println("Death test mode: pet starts sick and dies after 30 seconds.");
#elif defined(PET_SLEEP_TEST_MODE)
    pet.setMood(50);
    if (!save.save(pet)) Serial.println("Failed to save initial sleep test state.");
    Serial.println("Sleep test mode: entering sleep sets satiety to zero; mood recovers every 10 seconds, sickness starts after 20 seconds, and death follows 30 seconds later.");
#endif
    ui.init(now);
    lastEggCheckpointMinute = pet.lifeStage() == Pet::LifeStage::Egg ?
        pet.ageSeconds() / 60 : 0;
    appReady = true;
    if (!ble.init()) Serial.println("BLE init failed; pet remains available offline.");
    printUiState(Hardware::InputEvent::None);
    ui.render();
}

void Application::loop() {
    if (!appReady) return;

    const uint32_t now = millis();
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
    const Hardware::InputEvent event = input.update();
    if (kDebugUi && event != Hardware::InputEvent::None) {
        Serial.print("Physical/Input event: ");
        Serial.println(inputEventName(event));
    }
    const bool changed = ui.update(event, now);
    const uint32_t revisionBeforeAction = pet.displayRevision();
    const Pet::LifeStage stageBeforeAction = pet.lifeStage();
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
            if (pet.clean()) sound.playSuccess(); else sound.playFailure();
            break;
        case Ui::UiAction::Play:
            if (pet.play()) sound.playSuccess(); else sound.playFailure();
            break;
        case Ui::UiAction::Treat:
            if (pet.treat()) sound.playSuccess();
            else sound.playFailure();
            break;
        case Ui::UiAction::StartNormalSleep:
            if (startNormalSleep(now)) {
                sound.playConfirm();
                actionSavedImmediately = true;
            } else {
                sound.playFailure();
            }
            break;
        case Ui::UiAction::Wake:
            if (wakeFromNormalSleep(now)) {
                sound.playSuccess();
                actionSavedImmediately = true;
            } else {
                sound.playFailure();
            }
            break;
        case Ui::UiAction::AdoptNewEgg:
            if (adoptNewEgg(now)) sound.playSuccess();
            else sound.playFailure();
            break;
        case Ui::UiAction::DeleteMemorial:
            ui.onMemorialDeleteResult(deleteSelectedMemorial());
            break;
        case Ui::UiAction::None: break;
    }
    if (actionSavedImmediately) {
        // Entering and leaving sleep already wrote a verified checkpoint.
    } else if (pet.lifeStage() != stageBeforeAction) {
        if (!save.save(pet)) Serial.println("Failed to save pet growth stage.");
    } else if (pet.displayRevision() != revisionBeforeAction) {
        save.markDirty(now);
    }
    save.update(pet, now);
    if (changed) printUiState(event);
    sound.update();
    ui.render();
    ble.update(pet, now);
}

Application application;
}  // namespace

void setup() { application.setup(); }
void loop() { application.loop(); }
