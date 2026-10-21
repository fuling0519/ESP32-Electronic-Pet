#if defined(PET_TOUCH_READ_TEST_MODE)
#include <Arduino.h>
#include <driver/touch_sensor.h>
#include <esp_arduino_version.h>

namespace {
constexpr uint8_t kTouchPin = 32;
constexpr uint32_t kSampleIntervalMs = 200;
constexpr uint16_t kDefaultMeasureCycles = 0x1000;
constexpr uint16_t kLongMeasureCycles = 40000;
constexpr uint16_t kSleepCycles = 0x1000;
uint32_t sampledAt = 0;
uint32_t startedAt = 0;
bool announced = false;

void selectMeasurementMode(uint8_t mode) {
    const uint16_t measure = mode == 0 ? kDefaultMeasureCycles : kLongMeasureCycles;
    // Legacy IDF expresses a 1.7 V upper limit as 2.7 V minus 1.0 V.
    const touch_volt_atten_t attenuation = mode == 2 ? TOUCH_HVOLT_ATTEN_1V
                                                    : TOUCH_HVOLT_ATTEN_0V;
    // Call after the first touchRead: Core 2.0.17 lazily initializes the
    // peripheral and would otherwise overwrite these settings on first read.
    touchSetCycles(measure, kSleepCycles);
    const esp_err_t voltageResult = touch_pad_set_voltage(TOUCH_HVOLT_2V7,
                                                        TOUCH_LVOLT_0V5, attenuation);
    uint16_t actualSleep = 0;
    uint16_t actualMeasure = 0;
    const esp_err_t timingResult = touch_pad_get_meas_time(&actualSleep, &actualMeasure);
    if (voltageResult != ESP_OK || timingResult != ESP_OK ||
        actualSleep != kSleepCycles || actualMeasure != measure) {
        Serial.printf("Touch configuration ERROR: voltage=%d timing=%d measure=%u sleep=%u\n",
                      voltageResult, timingResult, static_cast<unsigned>(actualMeasure),
                      static_cast<unsigned>(actualSleep));
        return;
    }
    Serial.printf("Mode %u | measure=%u | sleep=%u | voltage=0.5-%s V\n",
                  static_cast<unsigned>(mode), static_cast<unsigned>(actualMeasure),
                  static_cast<unsigned>(actualSleep), mode == 2 ? "1.7" : "2.7");
    // Discard the previous measurement by waiting a normal print interval.
    sampledAt = millis();
}
}

// Isolated raw readings: this sketch does not access pet saves or BLE.
// Mode 0 is the pinned core's defaults; modes 1/2 change real measurements,
// never multiply the displayed raw value or alter production thresholds.
void setup() {
    Serial.begin(115200);
    (void)touchRead(kTouchPin);
    startedAt = millis();
}

void loop() {
    const uint32_t now = millis();
    if (!announced) {
        if (static_cast<uint32_t>(now - startedAt) < 1000) return;
        Serial.println("Touch Sensor Test - esp32dev_touch_read_test");
        Serial.printf("Arduino ESP32 Core: %u.%u.%u | GPIO: %u | Interval: %lu ms\n",
                      ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR,
                      ESP_ARDUINO_VERSION_PATCH, kTouchPin,
                      static_cast<unsigned long>(kSampleIntervalMs));
        Serial.println("Send 0: default ~0.5 ms, 0.5-2.7 V");
        Serial.println("Send 1: longer ~5 ms, 0.5-2.7 V");
        Serial.println("Send 2: longer ~5 ms, 0.5-1.7 V");
        selectMeasurementMode(0);
        announced = true;
        return;
    }
    if (Serial.available()) {
        const int command = Serial.read();
        if (command >= '0' && command <= '2') {
            selectMeasurementMode(static_cast<uint8_t>(command - '0'));
            return;
        }
    }
    if (static_cast<uint32_t>(now - sampledAt) < kSampleIntervalMs) return;
    sampledAt = now;
    Serial.printf("Touch Value: %u\n", touchRead(kTouchPin));
}
#endif
