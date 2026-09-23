#include "pet/PetData.h"

namespace Pet {

PetData::PetData()
    : satiety_(80),
      mood_(80),
      cleanliness_(100),
      level_(1),
      exp_(0),
      isSick_(false),
      isDead_(false) {}

uint8_t PetData::satiety() const { return satiety_; }
uint8_t PetData::mood() const { return mood_; }
uint8_t PetData::cleanliness() const { return cleanliness_; }
uint8_t PetData::level() const { return level_; }
uint16_t PetData::exp() const { return exp_; }
bool PetData::isSick() const { return isSick_; }
bool PetData::isDead() const { return isDead_; }

HungerState PetData::hungerState() const {
    if (satiety_ >= 76) return HungerState::Satisfied;
    if (satiety_ >= 51) return HungerState::SlightlyHungry;
    if (satiety_ >= 26) return HungerState::Hungry;
    if (satiety_ >= 11) return HungerState::VeryHungry;
    return HungerState::Starving;
}

MoodState PetData::moodState() const {
    if (mood_ >= 81) return MoodState::VeryHappy;
    if (mood_ >= 61) return MoodState::Happy;
    if (mood_ >= 41) return MoodState::Neutral;
    if (mood_ >= 21) return MoodState::Sad;
    return MoodState::VerySad;
}

CleanlinessState PetData::cleanlinessState() const {
    if (cleanliness_ >= 76) return CleanlinessState::Clean;
    if (cleanliness_ >= 51) return CleanlinessState::SlightlyDirty;
    if (cleanliness_ >= 26) return CleanlinessState::Dirty;
    return CleanlinessState::Filthy;
}

void PetData::setSatiety(int value) { satiety_ = clampNeedValue(value); }
void PetData::setMood(int value) { mood_ = clampNeedValue(value); }
void PetData::setCleanliness(int value) { cleanliness_ = clampNeedValue(value); }

void PetData::changeSatiety(int amount) { setSatiety(static_cast<int>(satiety_) + amount); }
void PetData::changeMood(int amount) { setMood(static_cast<int>(mood_) + amount); }
void PetData::changeCleanliness(int amount) {
    setCleanliness(static_cast<int>(cleanliness_) + amount);
}

void PetData::setLevel(uint8_t value) { level_ = value; }
void PetData::setExp(uint16_t value) { exp_ = value; }
void PetData::setSick(bool value) { isSick_ = value; }
void PetData::setDead(bool value) { isDead_ = value; }

uint8_t PetData::clampNeedValue(int value) {
    if (value < kMinNeedValue) return kMinNeedValue;
    if (value > kMaxNeedValue) return kMaxNeedValue;
    return static_cast<uint8_t>(value);
}

}  // namespace Pet
