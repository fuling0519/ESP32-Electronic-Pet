#pragma once
#include <stdint.h>

namespace Hardware {
enum class TouchEvent : uint8_t { None, Poke, PettingStarted, PettingReleased, Cancelled };

// Pure sampled-signal state machine. No GPIO, UI or game-state dependencies.
class TouchGesture {
public:
    void cancel();
    TouchEvent sample(uint16_t raw, uint32_t now, bool allowed);
    TouchEvent tick(uint32_t now, bool allowed);
private:
    enum class State : uint8_t { Blocked, Idle, Candidate, Active, Releasing, Petting, PettingReleasing };
    State state_ = State::Blocked;
    uint8_t count_ = 0;
    uint32_t startedAt_ = 0, releasedAt_ = 0, sampledAt_ = 0;
    bool sampled_ = false, low_ = false;
};
}  // namespace Hardware
