#include "hardware/TouchInput.h"
#include <Arduino.h>
#include "HardwareConfig.h"

namespace Hardware {
void TouchInput::init() {
    // Arduino's first channel initialization may wait internally. Warm it once
    // in setup, before the application starts its PetClock; never add a delay.
    (void)touchRead(HardwareConfig::Pins::Touch);
    // Core 2.0.17 lazily initializes touch hardware. Apply cycles afterwards
    // so its first-read defaults cannot overwrite the calibrated settings.
    // Keep its 0.5-2.7 V voltage range, matching diagnostic MODE 1.
    touchSetCycles(HardwareConfig::Touch::MeasureCycles, HardwareConfig::Touch::SleepCycles);
    gesture_ = TouchGesture{};
    sampledAt_ = debugAt_ = millis();
}

TouchEvent TouchInput::update(uint32_t now, bool allowed) {
    if (static_cast<uint32_t>(now - sampledAt_) >= HardwareConfig::Touch::SampleIntervalMs) {
        sampledAt_ = now;
        const uint16_t raw = touchRead(HardwareConfig::Pins::Touch);
        const TouchEvent event = gesture_.sample(raw, now, allowed);
        if (HardwareConfig::Touch::Debug && static_cast<uint32_t>(now - debugAt_) >= 1000) {
            debugAt_ = now;
            Serial.printf("Touch raw=%u base=%u allowed=%u\n", raw, gesture_.baseline(), allowed);
        }
        return event;
    }
    return gesture_.tick(now, allowed);
}
}  // namespace Hardware
