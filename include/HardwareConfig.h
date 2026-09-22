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
}  // namespace Pins

namespace Joystick {
constexpr uint8_t AdcResolutionBits = 12;
constexpr int DeadZone = 250;
constexpr int DirectionThreshold = 900;
// Confirm these directions with the assembled HW-504.
constexpr bool InvertX = false;
constexpr bool InvertY = false;
constexpr uint32_t DirectionRepeatDelayMs = 450;
constexpr uint32_t DirectionRepeatIntervalMs = 180;
constexpr uint32_t SwitchDebounceMs = 35;
constexpr uint32_t LongPressMs = 700;
}  // namespace Joystick

namespace Sound {
constexpr uint8_t LedcChannel = 0;
constexpr uint8_t LedcResolutionBits = 8;
}  // namespace Sound
}  // namespace HardwareConfig
