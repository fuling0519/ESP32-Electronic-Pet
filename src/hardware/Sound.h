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
    bool isPlaying() const { return playing_; }
    // Binary device preference: 0 = muted, 1 = enabled.
    static constexpr uint8_t kDefaultVolume = 1;
    static constexpr uint8_t kVolumeCount = 2;
    uint8_t volume() const { return volume_; }
    void setVolume(uint8_t level);
    void playConfirm(); void playCancel(); void playSuccess(); void playFailure(); void playHatch();
    void playDeath();
    void playLevelUp();
    void playCareReminder();
private:
    void startCurrentStep();
    void applyCurrentOutput();
    uint8_t volume_ = kDefaultVolume;
    const ToneStep* sequence_ = nullptr;
    size_t sequenceCount_ = 0;
    size_t sequenceIndex_ = 0;
    uint32_t stepStartedAt_ = 0;
    bool playing_ = false;
};
}  // namespace Hardware
