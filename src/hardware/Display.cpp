#include "hardware/Display.h"

#include <Wire.h>
#include <U8g2lib.h>

#include "HardwareConfig.h"

namespace Hardware {
namespace {
U8G2_SH1106_128X64_NONAME_F_HW_I2C sh1106(
    U8G2_R0, U8X8_PIN_NONE, HardwareConfig::Pins::OledScl,
    HardwareConfig::Pins::OledSda);
}  // namespace

uint8_t Display::scanI2c() {
    Wire.begin(HardwareConfig::Pins::OledSda, HardwareConfig::Pins::OledScl);
    detectedDeviceCount_ = 0;
    i2cAddress_ = 0;

    for (uint8_t address = 1; address < 127; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            if (detectedDeviceCount_ == 0) i2cAddress_ = address;
            ++detectedDeviceCount_;
        }
    }
    return detectedDeviceCount_;
}

bool Display::init() {
    Wire.begin(HardwareConfig::Pins::OledSda, HardwareConfig::Pins::OledScl);
    if (i2cAddress_ == 0 && scanI2c() == 0) return false;

    // A configured address takes precedence after the scanner has confirmed it.
    const uint8_t address = HardwareConfig::Oled::I2cAddress != 0
                                ? HardwareConfig::Oled::I2cAddress
                                : i2cAddress_;
    sh1106.setI2CAddress(static_cast<uint8_t>(address << 1));
    sh1106.begin();
    sh1106.clearBuffer();
    sh1106.setFont(u8g2_font_6x10_tf);
    initialized_ = true;
    i2cAddress_ = address;
    return true;
}

void Display::clear() {
    if (initialized_) sh1106.clearBuffer();
}

void Display::update() {
    if (initialized_) sh1106.sendBuffer();
}

void Display::drawText(int16_t x, int16_t y, const char* text) {
    if (initialized_ && text != nullptr) sh1106.drawStr(x, y, text);
}

void Display::drawFrame(int16_t x, int16_t y, int16_t width, int16_t height) {
    if (initialized_) sh1106.drawFrame(x, y, width, height);
}

void Display::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (initialized_) sh1106.drawLine(x1, y1, x2, y2);
}

bool Display::isInitialized() const { return initialized_; }
uint8_t Display::i2cAddress() const { return i2cAddress_; }
uint8_t Display::detectedDeviceCount() const { return detectedDeviceCount_; }
}  // namespace Hardware
