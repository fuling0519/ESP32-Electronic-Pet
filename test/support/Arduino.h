#pragma once
#include <stdint.h>
#define PROGMEM
#define pgm_read_byte(address) (*reinterpret_cast<const uint8_t*>(address))
struct SerialStub {
    template <typename... T> void print(T...) {}
    template <typename... T> void println(T...) {}
    template <typename... T> void printf(T...) {}
};
extern SerialStub Serial;
uint32_t millis();
double ledcSetup(uint8_t channel, double frequency, uint8_t resolution);
void ledcAttachPin(uint8_t pin, uint8_t channel);
double ledcWriteTone(uint8_t channel, double frequency);
