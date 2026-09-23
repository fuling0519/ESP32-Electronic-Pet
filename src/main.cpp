#include <Arduino.h>

#include "hardware/Display.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"
#include "storage/Save.h"
#include "ui/UiController.h"

Hardware::Display display;
Hardware::Input input;
Hardware::Sound sound;
Storage::Save save;
Ui::UiController ui(display, sound);

namespace {
constexpr bool kDebugUi = true;
bool appReady = false;

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

void printUiState(Hardware::InputEvent event) {
    if (!kDebugUi) return;
    (void)event;
    Serial.print("Screen: ");
    Serial.println(ui.screenName());
    if (ui.screen() == Ui::ScreenId::MainMenu) {
        Serial.printf("Menu index: %u\nSelected: %s\n", ui.menuIndex(), ui.selectedMenuItem());
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Phase 1A UI Navigation Test ===");
    if (!display.init()) {
        Serial.println("Display init failed.");
        return;
    }
    input.init();
    sound.init();
    save.init();
    ui.init(millis());
    appReady = true;
    printUiState(Hardware::InputEvent::None);
    ui.render();
}

void loop() {
    if (!appReady) return;

    const Hardware::InputEvent event = input.update();
    if (kDebugUi && event != Hardware::InputEvent::None) {
        Serial.print("Physical/Input event: ");
        Serial.println(inputEventName(event));
    }
    const bool changed = ui.update(event, millis());
    if (changed) printUiState(event);
    sound.update();
    ui.render();
}
