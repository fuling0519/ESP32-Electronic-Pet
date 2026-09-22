#include <Arduino.h>

#include "hardware/Input.h"
#include "hardware/Sound.h"

Hardware::Input input;
Hardware::Sound sound;

namespace {
constexpr uint32_t kAdcReportIntervalMs = 300;
uint32_t lastAdcReportAt = 0;

const char* eventName(Hardware::InputEvent event) {
    switch (event) {
        case Hardware::InputEvent::Up: return "UP";
        case Hardware::InputEvent::Down: return "DOWN";
        case Hardware::InputEvent::Left: return "LEFT";
        case Hardware::InputEvent::Right: return "RIGHT";
        case Hardware::InputEvent::Press: return "PRESS";
        case Hardware::InputEvent::LongPress: return "LONG_PRESS";
        case Hardware::InputEvent::None: return "";
    }
    return "";
}

void playSmokeTestTone(Hardware::InputEvent event) {
    switch (event) {
        case Hardware::InputEvent::Left: sound.playTone(262, 100); break;
        case Hardware::InputEvent::Down: sound.playTone(330, 100); break;
        case Hardware::InputEvent::Right: sound.playTone(392, 100); break;
        case Hardware::InputEvent::Up: sound.playTone(523, 100); break;
        case Hardware::InputEvent::Press: sound.playConfirm(); break;
        case Hardware::InputEvent::LongPress: sound.playSuccess(); break;
        case Hardware::InputEvent::None: break;
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Phase 1A Hardware Smoke Test ===");
    Serial.println("Joystick + Passive Buzzer");
    Serial.println("Calibrating joystick...");
    Serial.println("Please leave joystick centered.");
    input.init();
    sound.init();
    Serial.println("Calibration complete.");
    Serial.printf("Joystick center calibrated:\nX center: %d\nY center: %d\n",
                  input.centerX(), input.centerY());
}

void loop() {
    const Hardware::InputEvent event = input.update();
    if (event != Hardware::InputEvent::None) {
        Serial.print("Input: ");
        Serial.println(eventName(event));
        playSmokeTestTone(event);
    }

    sound.update();

    const uint32_t now = millis();
    if (now - lastAdcReportAt >= kAdcReportIntervalMs) {
        lastAdcReportAt = now;
        Serial.printf("X: %d | Y: %d\n", input.rawX(), input.rawY());
    }
}
