#pragma once

#include <stdint.h>

namespace Pet {

// Converts frequent millis() samples into whole seconds, preserving fractions.
// Unsigned subtraction also handles the normal millis() rollover.
class PetClock {
public:
    void reset(uint32_t nowMs);
    uint32_t consumeElapsedSeconds(uint32_t nowMs);

private:
    uint32_t lastWholeSecondMs_ = 0;
    bool initialized_ = false;
};

}  // namespace Pet
