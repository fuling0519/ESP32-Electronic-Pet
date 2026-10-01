#pragma once
#include <stdint.h>

namespace Power {
// Discard time beyond the cap, including its fractional remainder. A negative
// or backwards time basis is invalid, rather than an unknown offline duration.
inline bool sleepSettlement(int64_t startedUs, int64_t endedUs, uint32_t capSeconds,
                            uint32_t& seconds, uint32_t& remainderMs) {
    seconds = 0;
    remainderMs = 0;
    if (startedUs < 0 || endedUs < startedUs) return false;
    const int64_t elapsedUs = endedUs - startedUs;
    const int64_t capUs = static_cast<int64_t>(capSeconds) * 1000000LL;
    if (elapsedUs >= capUs) {
        seconds = capSeconds;
    } else {
        seconds = static_cast<uint32_t>(elapsedUs / 1000000LL);
        remainderMs = static_cast<uint32_t>((elapsedUs % 1000000LL) / 1000);
    }
    return true;
}
}
