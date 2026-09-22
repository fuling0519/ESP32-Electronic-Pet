#include "hardware/Display.h"

#include <Wire.h>
#include "HardwareConfig.h"

namespace Hardware {
void Display::init() {
    // Do not probe or assume an address until the SH1106 display arrives.
    Wire.begin(HardwareConfig::Pins::OledSda, HardwareConfig::Pins::OledScl);
    initialized_ = true;
}
void Display::clear() {}
void Display::update() {}
void Display::drawText(int16_t x, int16_t y, const char* text) {
    (void)x; (void)y; (void)text;
    // This preserves a library-independent upper-layer interface.
}
bool Display::isInitialized() const { return initialized_; }
}  // namespace Hardware
