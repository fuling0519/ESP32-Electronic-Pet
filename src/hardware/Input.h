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
    InputEvent updateSwitch(uint32_t now);
    int centerX_ = 0;
    int centerY_ = 0;
    InputEvent activeDirection_ = InputEvent::None;
    uint32_t directionStartedAt_ = 0;
    uint32_t lastDirectionEventAt_ = 0;
    bool lastRawSwitchPressed_ = false;
    bool switchPressed_ = false;
    bool longPressSent_ = false;
    uint32_t lastSwitchChangeAt_ = 0;
    uint32_t switchPressedAt_ = 0;
};
}  // namespace Hardware
