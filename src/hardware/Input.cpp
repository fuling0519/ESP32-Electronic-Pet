#include "hardware/Input.h"

#include <Arduino.h>
#include "HardwareConfig.h"

namespace Hardware {
namespace {
constexpr uint8_t kCenterSampleCount = 8;
int readAverage(int pin) {
    uint32_t total = 0;
    for (uint8_t i = 0; i < kCenterSampleCount; ++i) total += analogRead(pin);
    return static_cast<int>(total / kCenterSampleCount);
}
}  // namespace

void Input::init() {
    analogReadResolution(HardwareConfig::Joystick::AdcResolutionBits);
    pinMode(HardwareConfig::Pins::JoystickX, INPUT);
    pinMode(HardwareConfig::Pins::JoystickY, INPUT);
    pinMode(HardwareConfig::Pins::JoystickSwitch, INPUT_PULLUP);
    // Calibrate from the actual joystick; leave the stick centred at boot.
    centerX_ = readAverage(HardwareConfig::Pins::JoystickX);
    centerY_ = readAverage(HardwareConfig::Pins::JoystickY);
    lastRawSwitchPressed_ = digitalRead(HardwareConfig::Pins::JoystickSwitch) == LOW;
    switchPressed_ = lastRawSwitchPressed_;
    lastSwitchChangeAt_ = millis();
}

InputEvent Input::update() {
    const uint32_t now = millis();
    const InputEvent direction = readDirection();
    InputEvent event = InputEvent::None;
    if (direction != activeDirection_) {
        activeDirection_ = direction;
        directionStartedAt_ = now;
        if (direction != InputEvent::None) {
            lastDirectionEventAt_ = now;
            event = direction;
        }
    } else if (direction != InputEvent::None &&
               now - directionStartedAt_ >= HardwareConfig::Joystick::DirectionRepeatDelayMs &&
               now - lastDirectionEventAt_ >= HardwareConfig::Joystick::DirectionRepeatIntervalMs) {
        lastDirectionEventAt_ = now;
        event = direction;
    }
    const InputEvent switchEvent = updateSwitch(now);
    return switchEvent != InputEvent::None ? switchEvent : event;
}

InputEvent Input::readDirection() const {
    int x = analogRead(HardwareConfig::Pins::JoystickX) - centerX_;
    int y = analogRead(HardwareConfig::Pins::JoystickY) - centerY_;
    if (HardwareConfig::Joystick::InvertX) x = -x;
    if (HardwareConfig::Joystick::InvertY) y = -y;
    const int absX = abs(x);
    const int absY = abs(y);
    if (absX <= HardwareConfig::Joystick::DeadZone && absY <= HardwareConfig::Joystick::DeadZone) return InputEvent::None;
    // Choose the dominant axis so diagonals never create two UI events.
    if (absX >= absY && absX >= HardwareConfig::Joystick::DirectionThreshold) return x < 0 ? InputEvent::Left : InputEvent::Right;
    if (absY > absX && absY >= HardwareConfig::Joystick::DirectionThreshold) return y < 0 ? InputEvent::Up : InputEvent::Down;
    return InputEvent::None;
}

InputEvent Input::updateSwitch(uint32_t now) {
    const bool rawPressed = digitalRead(HardwareConfig::Pins::JoystickSwitch) == LOW;
    if (rawPressed != lastRawSwitchPressed_) {
        lastRawSwitchPressed_ = rawPressed;
        lastSwitchChangeAt_ = now;
    }
    if (rawPressed != switchPressed_ && now - lastSwitchChangeAt_ >= HardwareConfig::Joystick::SwitchDebounceMs) {
        switchPressed_ = rawPressed;
        if (switchPressed_) {
            switchPressedAt_ = now;
            longPressSent_ = false;
        } else if (!longPressSent_) return InputEvent::Press;
    }
    if (switchPressed_ && !longPressSent_ && now - switchPressedAt_ >= HardwareConfig::Joystick::LongPressMs) {
        longPressSent_ = true;
        return InputEvent::LongPress;
    }
    return InputEvent::None;
}
int Input::centerX() const { return centerX_; }
int Input::centerY() const { return centerY_; }
int Input::rawX() const { return analogRead(HardwareConfig::Pins::JoystickX); }
int Input::rawY() const { return analogRead(HardwareConfig::Pins::JoystickY); }
}  // namespace Hardware
