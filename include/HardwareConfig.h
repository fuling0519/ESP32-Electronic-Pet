#pragma once

#include <stdint.h>

// Keep board-specific pins and tuning values here so feature modules never
// need to know the physical wiring.
namespace HardwareConfig {

namespace Pins {
constexpr int OledSda = 21;
constexpr int OledScl = 22;
constexpr int JoystickX = 34;
constexpr int JoystickY = 35;
constexpr int JoystickSwitch = 26;
constexpr int Buzzer = 27;
constexpr int Touch = 32; // Classic ESP32 T9; not a wake source.
}  // namespace Pins

namespace Oled {
constexpr uint8_t Width = 128;
constexpr uint8_t Height = 64;
// Confirmed by the Phase 1A OLED smoke-test I2C scanner.
constexpr uint8_t I2cAddress = 0x3C;
}  // namespace Oled

namespace Joystick {
constexpr uint8_t AdcResolutionBits = 12;
// Returning inside this zone re-arms a discrete direction gesture.
constexpr int DeadZone = 250;
// A gesture begins only after exceeding this larger threshold.
constexpr int DirectionThreshold = 900;
// Confirm these directions with the assembled HW-504.
constexpr bool InvertX = false;
constexpr bool InvertY = false;
constexpr uint32_t InitialRepeatDelayMs = 400;
constexpr uint32_t RepeatIntervalMs = 150;
constexpr uint32_t SwitchDebounceMs = 35;
constexpr uint32_t LongPressMs = 700;
}  // namespace Joystick

namespace Sound {
constexpr uint8_t LedcChannel = 0;
constexpr uint8_t LedcResolutionBits = 8;
}  // namespace Sound
namespace Touch {
// MODE 1: measurement cycles use the ESP32 8 MHz clock (~5 ms).
// Sleep cycles use RTC_SLOW_CLK; keep the original inter-measurement setting.
constexpr uint16_t MeasureCycles = 40000;
constexpr uint16_t SleepCycles = 0x1000;
// The released reading drifts between days (MODE 1: 718-745 on 2026-10-22,
// 640-648 on 2026-10-25), so thresholds follow a learned released baseline
// instead of fixed values. Touched readings sat 30-170 below it both days.
// Touch: raw < baseline - TouchDrop. Release: raw > baseline - ReleaseDrop.
constexpr uint16_t TouchDrop = 20;
constexpr uint16_t ReleaseDrop = 12;
// While idle and not touched the baseline moves 1/2^BaselineShift of the way
// to each reading (1/16 per 30 ms sample: about 1 s to follow a step).
constexpr uint8_t BaselineShift = 4;
// A reading that stays "touched" this long is taken as a baseline step down:
// the gesture is cancelled and the baseline restarts from the current reading.
constexpr uint32_t RebaselineMs = 30000;
constexpr uint8_t ConfirmSamples = 3;
constexpr uint32_t SampleIntervalMs = 30;
constexpr uint32_t MaximumSampleGapMs = 150;
constexpr uint32_t MinPokeMs = 100;
constexpr uint32_t PettingMs = 700;
constexpr uint32_t PettingReleaseGraceMs = 120;
constexpr uint32_t PokeFirstPhaseMs = 160;
constexpr uint32_t PokeAnimationMs = 500;
constexpr uint32_t PettingMaxAnimationMs = 5000;
constexpr uint32_t PettingTailMs = 250;
constexpr uint8_t BirdFramesPerSecond = 5;
constexpr uint8_t WyvernFramesPerSecond = 7;
constexpr bool Debug = true;
}  // namespace Touch
namespace Sleep {
#if defined(PET_DEEP_SLEEP_TEST_MODE)
constexpr uint32_t TimerSeconds = 60;
#else
// Zero disables timer wakeup. Players wake the production pet with SW only.
constexpr uint32_t TimerSeconds = 0;
#endif
constexpr uint32_t ReleaseStableMs = 100;
constexpr uint32_t ReleaseTimeoutMs = 10000;
constexpr uint32_t MaximumElapsedSeconds = 48UL * 60 * 60;
}
}  // namespace HardwareConfig
