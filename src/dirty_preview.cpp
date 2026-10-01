#if defined(PET_DIRTY_PREVIEW_MODE)
#include <Arduino.h>
#include <esp_system.h>
#include "hardware/Display.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"
#include "pet/PetData.h"
#include "storage/Memorials.h"
#include "ui/UiController.h"

namespace {
Hardware::Display display;
Hardware::Input input;
Hardware::Sound sound;
Pet::PetData pet;
Storage::Memorials memorials;
Ui::UiController ui(display, sound, pet, memorials, esp_random);
constexpr uint8_t values[] = {100, 75, 50, 25};
uint32_t changedAt;
uint8_t stage = 0;
bool paused = false;
bool ready = false;
}

// Visual-only preview: no NVS initialization, reads, or writes.
void setup() {
    Serial.begin(115200);
    if (!display.init()) return;
    input.init();
    sound.init();
    changedAt = millis();
    ui.init(changedAt);
    ready = true;
    Serial.println("Dirt preview: 100 -> 75 -> 50 -> 25 every 4s. Left/right: stage; press: pause; up: eat; down: sleep/wake.");
}

void loop() {
    if (!ready) return;
    const uint32_t now = millis();
    const auto event = input.update();
    bool next = !paused && static_cast<uint32_t>(now - changedAt) >= 4000;
    if (event == Hardware::InputEvent::Press) paused = !paused;
    if (event == Hardware::InputEvent::Left || event == Hardware::InputEvent::Right) {
        stage = (stage + (event == Hardware::InputEvent::Left ? 3 : 1)) % 4;
        next = false;
        changedAt = now;
        pet.setCleanliness(values[stage]);
    }
    if (next) {
        stage = (stage + 1) % 4;
        changedAt = now;
        pet.setCleanliness(values[stage]);
        Serial.printf("Cleanliness=%u; stains=%u\n", values[stage], stage);
    }
    if (event == Hardware::InputEvent::Up && ui.screen() == Ui::ScreenId::Home) ui.onFeedSucceeded(now);
    if (event == Hardware::InputEvent::Down) {
        if (pet.sleepMode() == Pet::SleepMode::Normal) {
            pet.wake();
            ui.onWakeSucceeded();
        } else if (ui.screen() == Ui::ScreenId::Home && pet.beginNormalSleep()) ui.onSleepStarted(now);
    }
    ui.update(Hardware::InputEvent::None, now);
    sound.update();
    ui.render();
}
#endif
