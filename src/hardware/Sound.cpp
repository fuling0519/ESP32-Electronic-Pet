#include "hardware/Sound.h"

#include <Arduino.h>
#include "HardwareConfig.h"

namespace Hardware {
namespace {
const ToneStep kConfirm[] = {{1047, 70}};
const ToneStep kCancel[] = {{392, 100}};
const ToneStep kSuccess[] = {{784, 80}, {1047, 120}};
const ToneStep kFailure[] = {{330, 120}, {262, 160}};
const ToneStep kHatch[] = {{523, 70}, {659, 70}, {784, 110}};
}  // namespace
void Sound::init() {
    ledcSetup(HardwareConfig::Sound::LedcChannel, 2000, HardwareConfig::Sound::LedcResolutionBits);
    ledcAttachPin(HardwareConfig::Pins::Buzzer, HardwareConfig::Sound::LedcChannel);
    stopTone();
}
void Sound::update() {
    if (!playing_ || millis() - stepStartedAt_ < sequence_[sequenceIndex_].durationMs) return;
    ++sequenceIndex_;
    if (sequenceIndex_ >= sequenceCount_) { stopTone(); return; }
    startCurrentStep();
}
void Sound::playTone(uint16_t frequency, uint32_t durationMs) {
    static ToneStep singleTone;
    singleTone = {frequency, static_cast<uint16_t>(durationMs > UINT16_MAX ? UINT16_MAX : durationMs)};
    playSequence(&singleTone, 1);
}
void Sound::playSequence(const ToneStep* steps, size_t count) {
    if (steps == nullptr || count == 0) { stopTone(); return; }
    sequence_ = steps; sequenceCount_ = count; sequenceIndex_ = 0; playing_ = true; startCurrentStep();
}
void Sound::stopTone() {
    ledcWriteTone(HardwareConfig::Sound::LedcChannel, 0);
    sequence_ = nullptr; sequenceCount_ = 0; sequenceIndex_ = 0; playing_ = false;
}
void Sound::playConfirm() { playSequence(kConfirm, 1); }
void Sound::playCancel() { playSequence(kCancel, 1); }
void Sound::playSuccess() { playSequence(kSuccess, 2); }
void Sound::playFailure() { playSequence(kFailure, 2); }
void Sound::playHatch() { playSequence(kHatch, 3); }
void Sound::startCurrentStep() {
    ledcWriteTone(HardwareConfig::Sound::LedcChannel, sequence_[sequenceIndex_].frequency);
    stepStartedAt_ = millis();
}
}  // namespace Hardware
