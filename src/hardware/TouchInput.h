#pragma once
#include "hardware/TouchGesture.h"

namespace Hardware {
class TouchInput {
public:
    void init();
    TouchEvent update(uint32_t now, bool allowed);
    void cancel() { gesture_.cancel(); }
private:
    TouchGesture gesture_;
    uint32_t sampledAt_ = 0, debugAt_ = 0;
};
}  // namespace Hardware
