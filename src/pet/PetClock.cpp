#include "pet/PetClock.h"

namespace Pet {

void PetClock::reset(uint32_t nowMs) {
    lastWholeSecondMs_ = nowMs;
    initialized_ = true;
}

uint32_t PetClock::consumeElapsedSeconds(uint32_t nowMs) {
    if (!initialized_) { reset(nowMs); return 0; }
    const uint32_t seconds = (nowMs - lastWholeSecondMs_) / 1000;
    lastWholeSecondMs_ += seconds * 1000;
    return seconds;
}

}  // namespace Pet
