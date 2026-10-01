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
