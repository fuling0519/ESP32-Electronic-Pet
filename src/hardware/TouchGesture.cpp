#include "hardware/TouchGesture.h"
#include "HardwareConfig.h"

namespace Hardware {
namespace T = HardwareConfig::Touch;

void TouchGesture::cancel() {
    count_ = 0;
    state_ = State::Blocked;
    low_ = false;
}

TouchEvent TouchGesture::sample(uint16_t raw, uint32_t now, bool allowed) {
    const bool gap = sampled_ && static_cast<uint32_t>(now - sampledAt_) > T::MaximumSampleGapMs;
    if (gap) cancel();
    sampled_ = true;
    sampledAt_ = now;
    if (!allowed) cancel();
    if (!raw) { cancel(); return TouchEvent::Cancelled; }
    low_ = raw < T::TouchBelow;
    const bool high = raw > T::ReleaseAbove;
    if (state_ == State::Blocked) {
        count_ = high ? count_ + 1 : 0;
        if (count_ >= T::ConfirmSamples) { state_ = State::Idle; count_ = 0; }
        return gap ? TouchEvent::Cancelled : TouchEvent::None;
    }
    if (!allowed) return TouchEvent::None;
    switch (state_) {
        case State::Idle:
            if (low_) { state_ = State::Candidate; startedAt_ = now; count_ = 1; }
            break;
        case State::Candidate:
            if (!low_) { state_ = State::Idle; count_ = 0; }
            else if (++count_ >= T::ConfirmSamples) { state_ = State::Active; count_ = 0; }
            break;
        case State::Active:
        case State::Petting:
            if (high) {
                state_ = state_ == State::Active ? State::Releasing : State::PettingReleasing;
                releasedAt_ = now; count_ = 1;
            }
            break;
        case State::Releasing:
        case State::PettingReleasing:
            if (low_) {
                state_ = state_ == State::Releasing ? State::Active : State::Petting;
                count_ = 0;
            } else {
                count_ = high ? (count_ < T::ConfirmSamples ? count_ + 1 : count_) : 0;
                const bool petting = state_ == State::PettingReleasing;
                if (count_ >= T::ConfirmSamples &&
                    (!petting || static_cast<uint32_t>(now - releasedAt_) >= T::PettingReleaseGraceMs)) {
                    state_ = State::Idle; count_ = 0;
                    const uint32_t duration = releasedAt_ - startedAt_;
                    if (petting) return TouchEvent::PettingReleased;
                    // A long interval never observed as Started is ambiguous:
                    // do not invent a delayed petting event or downgrade it.
                    if (duration >= T::MinPokeMs && duration < T::PettingMs) return TouchEvent::Poke;
                }
            }
            break;
        case State::Blocked: break;
    }
    return tick(now, allowed);
}

TouchEvent TouchGesture::tick(uint32_t now, bool allowed) {
    if (!allowed) { cancel(); return TouchEvent::None; }
    if (sampled_ && static_cast<uint32_t>(now - sampledAt_) > T::MaximumSampleGapMs) {
        cancel(); sampled_ = false; return TouchEvent::Cancelled;
    }
    if (state_ == State::Active && low_ && static_cast<uint32_t>(now - startedAt_) >= T::PettingMs) {
        state_ = State::Petting;
        return TouchEvent::PettingStarted;
    }
    return TouchEvent::None;
}
}  // namespace Hardware
