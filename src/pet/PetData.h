#pragma once

#include <stdint.h>

namespace Pet {

enum class HungerState : uint8_t {
    Satisfied,
    SlightlyHungry,
    Hungry,
    VeryHungry,
    Starving,
};

enum class MoodState : uint8_t {
    VeryHappy,
    Happy,
    Neutral,
    Sad,
    VerySad,
};

enum class CleanlinessState : uint8_t {
    Clean,
    SlightlyDirty,
    Dirty,
    Filthy,
};

// Core pet state only. Gameplay systems update the bounded needs through the
// change/set methods instead of modifying their values directly.
class PetData {
public:
    static constexpr uint8_t kMinNeedValue = 0;
    static constexpr uint8_t kMaxNeedValue = 100;

    PetData();

    uint8_t satiety() const;
    uint8_t mood() const;
    uint8_t cleanliness() const;
    uint8_t level() const;
    uint16_t exp() const;
    bool isSick() const;
    HungerState hungerState() const;
    MoodState moodState() const;
    CleanlinessState cleanlinessState() const;

    void setSatiety(int value);
    void setMood(int value);
    void setCleanliness(int value);
    void changeSatiety(int amount);
    void changeMood(int amount);
    void changeCleanliness(int amount);

    void setLevel(uint8_t value);
    void setExp(uint16_t value);
    void setSick(bool value);

private:
    static uint8_t clampNeedValue(int value);

    uint8_t satiety_;
    uint8_t mood_;
    uint8_t cleanliness_;
    uint8_t level_;
    uint16_t exp_;
    bool isSick_;
};

}  // namespace Pet
