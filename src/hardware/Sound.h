#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Hardware {
struct ToneStep { uint16_t frequency; uint16_t durationMs; };
class Sound {
public:
    void init();
    void update();
    void playTone(uint16_t frequency, uint32_t durationMs);
    void playSequence(const ToneStep* steps, size_t count);
    void stopTone();
    void playConfirm(); void playCancel(); void playSuccess(); void playFailure(); void playHatch();
    void playDeath();
private:
    void startCurrentStep();
    const ToneStep* sequence_ = nullptr;
    size_t sequenceCount_ = 0;
    size_t sequenceIndex_ = 0;
    uint32_t stepStartedAt_ = 0;
    bool playing_ = false;
};
}  // namespace Hardware
