#pragma once

#include <stdint.h>

namespace Hardware {
enum class InputEvent { None, Up, Down, Left, Right, Press, LongPress };

class Input {
public:
    void init();
    InputEvent update();
    int centerX() const;
    int centerY() const;
    int rawX() const;
    int rawY() const;
private:
    InputEvent readDirection() const;
    bool isNeutral() const;
    bool isDirectionHeld(InputEvent direction) const;
    InputEvent updateSwitch(uint32_t now);
    int centerX_ = 0;
    int centerY_ = 0;
    bool directionLocked_ = false;
    InputEvent lockedDirection_ = InputEvent::None;
    uint32_t holdStartedAt_ = 0;
    uint32_t lastRepeatAt_ = 0;
    bool lastRawSwitchPressed_ = false;
    bool switchPressed_ = false;
    bool longPressSent_ = false;
    uint32_t lastSwitchChangeAt_ = 0;
    uint32_t switchPressedAt_ = 0;
};
}  // namespace Hardware
