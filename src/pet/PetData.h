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
    static constexpr uint32_t kSatietyDecaySeconds = 600;
    static constexpr uint32_t kCleanlinessDecaySeconds = 900;
    static constexpr uint32_t kMoodDecaySeconds = 1200;
    static constexpr uint32_t kSicknessExposureSeconds = 6UL * 60 * 60;
    static constexpr uint8_t kFeedAmount = 20;
    static constexpr uint8_t kCleanAmount = 30;

    PetData();

    uint8_t satiety() const;
    uint8_t mood() const;
    uint8_t cleanliness() const;
    uint8_t level() const;
    uint16_t exp() const;
    bool isSick() const;
    bool isDead() const;
    uint64_t ageSeconds() const;
    uint32_t displayRevision() const;
    uint32_t dangerSeconds() const;
    uint32_t sickAwakeSeconds() const;
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
    void setDead(bool value);
    // Advances awake time in whole seconds. Keep the remainders with the pet
    // so future recovery can resume without losing partial intervals.
    void advanceSeconds(uint32_t seconds);
    bool feed();
    bool clean();
    bool treat();

private:
    static uint8_t clampNeedValue(int64_t value);

    uint8_t satiety_;
    uint8_t mood_;
    uint8_t cleanliness_;
    uint8_t level_;
    uint16_t exp_;
    bool isSick_;
    bool isDead_;
    uint64_t ageSeconds_;
    uint32_t satietyRemainderSeconds_;
    uint32_t cleanlinessRemainderSeconds_;
    uint32_t moodRemainderSeconds_;
    uint32_t dangerSeconds_;
    uint32_t sickAwakeSeconds_;
    uint32_t displayRevision_;
};

}  // namespace Pet
