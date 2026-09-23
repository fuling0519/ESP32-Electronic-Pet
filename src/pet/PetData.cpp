#include "pet/PetData.h"

namespace Pet {

PetData::PetData()
    : satiety_(80),
      mood_(80),
      cleanliness_(100),
      level_(1),
      exp_(0),
      isSick_(false) {}

uint8_t PetData::satiety() const { return satiety_; }
uint8_t PetData::mood() const { return mood_; }
uint8_t PetData::cleanliness() const { return cleanliness_; }
uint8_t PetData::level() const { return level_; }
uint16_t PetData::exp() const { return exp_; }
bool PetData::isSick() const { return isSick_; }

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

uint8_t PetData::clampNeedValue(int value) {
    if (value < kMinNeedValue) return kMinNeedValue;
    if (value > kMaxNeedValue) return kMaxNeedValue;
    return static_cast<uint8_t>(value);
}

}  // namespace Pet
