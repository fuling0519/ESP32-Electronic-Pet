#include <Arduino.h>

#include "hardware/Display.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"

Hardware::Display display;
Hardware::Input input;
Hardware::Sound sound;

namespace {
void drawSmokeTestScreen() {
    char addressText[16];
    snprintf(addressText, sizeof(addressText), "I2C: 0x%02X", display.i2cAddress());

    display.clear();
    display.drawFrame(0, 0, 128, 64);
    display.drawText(29, 14, "OLED TEST");
    display.drawLine(8, 19, 119, 19);
    display.drawText(23, 34, "SH1106 128x64");
    display.drawText(35, 47, addressText);
    display.drawFrame(50, 51, 28, 10);
    display.drawText(57, 60, "OK");
    display.update();
}
}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Phase 1A OLED Smoke Test ===");
    Serial.println();
    Serial.println("Initializing I2C...");
    Serial.println("Scanning I2C...");
    const uint8_t deviceCount = display.scanI2c();
    if (deviceCount == 0) {
        Serial.println("No I2C device found.");
        return;
    }

    Serial.printf("OLED found at 0x%02X\n", display.i2cAddress());
    if (deviceCount > 1) {
        Serial.printf("Warning: %u I2C devices found.\n", deviceCount);
    }
    Serial.println();
    Serial.println("Initializing SH1106...");
    if (!display.init()) {
        Serial.println("SH1106 init failed.");
        return;
    }
    Serial.println("Display init: OK");
    Serial.println("Resolution: 128x64");
    drawSmokeTestScreen();
    Serial.println("OLED smoke test ready.");
}

void loop() {
    // OLED smoke test intentionally has no game or UI loop.
}
