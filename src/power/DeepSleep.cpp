#include "power/DeepSleep.h"
#include "power/SleepSettlement.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <sys/time.h>
#include <stddef.h>
#include "HardwareConfig.h"

namespace Power {
namespace {
struct Retained {
    uint32_t magic;
    uint32_t timerSeconds;
    Pet::PetSnapshotV1 pet;
    uint32_t moodRemainder;
    int centerX, centerY;
    int64_t startedUs;
    uint32_t checksum;
};
RTC_DATA_ATTR Retained state{};
constexpr uint32_t kMagic = 0x44535032; // Recovery remainder now stores progress units.
int64_t nowUs() {
    timeval value{};
    gettimeofday(&value, nullptr);
    return static_cast<int64_t>(value.tv_sec) * 1000000LL + value.tv_usec;
}
uint32_t checksum() {
    const auto* bytes = reinterpret_cast<const uint8_t*>(&state);
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < offsetof(Retained, checksum); ++i) hash = (hash ^ bytes[i]) * 16777619U;
    return hash;
}
bool valid() {
    const auto cause = esp_sleep_get_wakeup_cause();
    return esp_reset_reason() == ESP_RST_DEEPSLEEP &&
        (cause == ESP_SLEEP_WAKEUP_EXT0 ||
         (HardwareConfig::Sleep::TimerSeconds > 0 && cause == ESP_SLEEP_WAKEUP_TIMER)) &&
        state.magic == kMagic && state.timerSeconds == HardwareConfig::Sleep::TimerSeconds &&
        state.checksum == checksum() && Pet::isValidPetSnapshot(state.pet) &&
        state.pet.sleepMode == Pet::SleepMode::Deep &&
        state.moodRemainder < Pet::PetData::kSleepMoodRecoveryProgress &&
        state.centerX > 0 && state.centerX < 4095 && state.centerY > 0 && state.centerY < 4095;
}
}
void releaseWakePins() {
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(static_cast<gpio_num_t>(HardwareConfig::Pins::Buzzer));
    rtc_gpio_deinit(static_cast<gpio_num_t>(HardwareConfig::Pins::JoystickSwitch));
}
bool retainedCalibration(int& x, int& y) {
    if (!valid()) return false;
    x = state.centerX; y = state.centerY;
    return true;
}
bool resume(Pet::PetData& pet, uint32_t& remainderMs) {
    remainderMs = 0;
    if (!valid() || pet.petId() != state.pet.petId ||
        pet.sleepMode() != Pet::SleepMode::Deep || pet.ageSeconds() != state.pet.ageSeconds) {
        state.magic = 0;
        return false;
    }
    uint32_t seconds = 0;
    if (!sleepSettlement(state.startedUs, nowUs(), HardwareConfig::Sleep::MaximumElapsedSeconds,
                         seconds, remainderMs)) {
        Serial.println("Deep sleep time invalid; no elapsed time applied.");
        state.magic = 0;
        return false;
    }
    if (!pet.restore(state.pet) || !pet.restoreSleepRecoveryProgress(state.moodRemainder)) {
        state.magic = 0;
        return false;
    }
    // Consume retained state before performing NVS writes or starting BLE.
    state.magic = 0;
    pet.advanceSleepSeconds(seconds);
    if (!pet.isDead()) pet.wake();
    Serial.printf("Deep sleep resumed: reason=%s elapsed=%lu seconds age=%llu mood=%u\n",
        esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER ? "timer" : "SW",
        static_cast<unsigned long>(seconds), static_cast<unsigned long long>(pet.ageSeconds()), pet.mood());
    return true;
}
bool configure() {
    const auto sw = static_cast<gpio_num_t>(HardwareConfig::Pins::JoystickSwitch);
    if (esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL) != ESP_OK ||
        rtc_gpio_init(sw) != ESP_OK || rtc_gpio_set_direction(sw, RTC_GPIO_MODE_INPUT_ONLY) != ESP_OK ||
        rtc_gpio_pullup_en(sw) != ESP_OK || rtc_gpio_pulldown_dis(sw) != ESP_OK ||
        esp_sleep_enable_ext0_wakeup(sw, 0) != ESP_OK) {
        cancel(); return false;
    }
    if (HardwareConfig::Sleep::TimerSeconds > 0 &&
        esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(HardwareConfig::Sleep::TimerSeconds) * 1000000ULL) != ESP_OK) {
        cancel(); return false;
    }
    if (rtc_gpio_get_level(sw) == 0) { cancel(); return false; }
    return true;
}
void cancel() {
    state.magic = 0;
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    rtc_gpio_deinit(static_cast<gpio_num_t>(HardwareConfig::Pins::JoystickSwitch));
}
bool wakePressed() {
    return rtc_gpio_get_level(static_cast<gpio_num_t>(HardwareConfig::Pins::JoystickSwitch)) == 0;
}
void retain(const Pet::PetData& pet, int x, int y, uint32_t remainderMs) {
    state = {};
    state.magic = kMagic;
    state.timerSeconds = HardwareConfig::Sleep::TimerSeconds;
    state.pet = pet.snapshot();
    state.moodRemainder = pet.sleepRecoveryProgress();
    state.centerX = x; state.centerY = y;
    state.startedUs = nowUs() - static_cast<int64_t>(remainderMs) * 1000;
    state.checksum = checksum();
}
void enter() {
    const auto buzzer = static_cast<gpio_num_t>(HardwareConfig::Pins::Buzzer);
    ledcDetachPin(HardwareConfig::Pins::Buzzer);
    pinMode(HardwareConfig::Pins::Buzzer, OUTPUT);
    digitalWrite(HardwareConfig::Pins::Buzzer, LOW);
    gpio_hold_en(buzzer);
    gpio_deep_sleep_hold_en();
    if (HardwareConfig::Sleep::TimerSeconds == 0) {
        Serial.printf("Deep sleep entering: SW GPIO%d only, settlement cap=48 hours\n", HardwareConfig::Pins::JoystickSwitch);
    } else Serial.printf("Deep sleep entering: timer=%lu seconds, SW GPIO%d\n",
        static_cast<unsigned long>(HardwareConfig::Sleep::TimerSeconds), HardwareConfig::Pins::JoystickSwitch);
    Serial.flush();
    esp_deep_sleep_start();
}
}
