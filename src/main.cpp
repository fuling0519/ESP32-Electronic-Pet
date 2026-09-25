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
    save.init();
    const uint32_t now = millis();
    petClock.reset(now);
#if defined(PET_DEATH_TEST_MODE)
    pet.setSick(true);
    Serial.println("Death test mode: pet starts sick and dies after 30 seconds.");
#endif
    ui.init(now);
    appReady = true;
    printUiState(Hardware::InputEvent::None);
    ui.render();
}

void Application::loop() {
    if (!appReady) return;

    const uint32_t now = millis();
    pet.advanceSeconds(petClock.consumeElapsedSeconds(now));
    const Hardware::InputEvent event = input.update();
    if (kDebugUi && event != Hardware::InputEvent::None) {
        Serial.print("Physical/Input event: ");
        Serial.println(inputEventName(event));
    }
    const bool changed = ui.update(event, now);
    switch (ui.takeAction()) {
        case Ui::UiAction::Feed: pet.feed(); sound.playSuccess(); break;
        case Ui::UiAction::Clean: pet.clean(); sound.playSuccess(); break;
        case Ui::UiAction::Treat:
            if (pet.treat()) sound.playSuccess();
            else sound.playFailure();
            break;
        case Ui::UiAction::None: break;
    }
    if (changed) printUiState(event);
    sound.update();
    ui.render();
}

Application application;
}  // namespace

void setup() { application.setup(); }
void loop() { application.loop(); }
