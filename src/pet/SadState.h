#pragma once

#include <stdint.h>

#include "pet/PetData.h"

namespace Pet {

// Sad reasons with hysteresis, shared by the UI (sad image, care reminder) and BLE status.
// Mood enters at <=40 and leaves at >=45; satiety and cleanliness enter at <=25 and leave
// at >=30; sickness has no buffer. Eggs and ended pets are never sad. A new pet id resets
// the buffers so a previous pet's state never carries over.
class SadState {
public:
    // Returns true when the pet id changed and the buffers were reset.
    bool update(const PetData& pet) {
        const bool changed = petId_ != pet.petId();
        if (changed) reset(pet.petId());
        const bool eligible = !pet.isEnded() && pet.lifeStage() != LifeStage::Egg;
        sick_ = eligible && pet.isSick();
        mood_ = eligible && (mood_ ? pet.mood() < 45 : pet.mood() <= 40);
        satiety_ = eligible && (satiety_ ? pet.satiety() < 30 : pet.satiety() <= 25);
        cleanliness_ = eligible && (cleanliness_ ? pet.cleanliness() < 30 : pet.cleanliness() <= 25);
        return changed;
    }
    void reset(uint64_t petId) { petId_ = petId; mood_ = satiety_ = cleanliness_ = sick_ = false; }
    bool mood() const { return mood_; }
    bool satiety() const { return satiety_; }
    bool cleanliness() const { return cleanliness_; }
    bool sick() const { return sick_; }
    bool any() const { return mood_ || satiety_ || cleanliness_ || sick_; }
    uint64_t petId() const { return petId_; }

private:
    uint64_t petId_ = 0;
    bool mood_ = false, satiety_ = false, cleanliness_ = false, sick_ = false;
};

}  // namespace Pet
