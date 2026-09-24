#pragma once

#include <stdint.h>

namespace Hardware {
class Display {
public:
    uint8_t scanI2c();
    bool init();
    void clear();
    void update();
    void drawText(int16_t x, int16_t y, const char* text);
    // 5-pixel advance, 7-pixel font height; y is the text baseline.
    void drawSmallText(int16_t x, int16_t y, const char* text);
    void drawStatusText(int16_t x, int16_t y, const char* utf8);
    uint16_t statusTextWidth(const char* utf8);
    // cht2bitmap row-major, MSB-first bitmap; zero bits are visible strokes.
    void drawGlyph(int16_t x, int16_t y, const uint8_t* bitmap,
                   uint8_t width, uint8_t height);
    void drawFrame(int16_t x, int16_t y, int16_t width, int16_t height);
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    bool isInitialized() const;
    uint8_t i2cAddress() const;
    uint8_t detectedDeviceCount() const;
private:
    bool initialized_ = false;
    uint8_t i2cAddress_ = 0;
    uint8_t detectedDeviceCount_ = 0;
};
}  // namespace Hardware
