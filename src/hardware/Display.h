#pragma once

#include <stdint.h>

namespace Hardware {
// SH1106 drawing is added after its address and orientation are confirmed.
class Display {
public:
    void init();
    void clear();
    void update();
    void drawText(int16_t x, int16_t y, const char* text);
    bool isInitialized() const;
private:
    bool initialized_ = false;
};
}  // namespace Hardware
