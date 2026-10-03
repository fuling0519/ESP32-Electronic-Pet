#pragma once

#include <stdint.h>

#include "pet/PetSnapshot.h"

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
    static constexpr uint8_t kFeedExpReward = 3;
    static constexpr uint8_t kCleanExpReward = 3;
    static constexpr uint8_t kTreatExpReward = 5;
    static constexpr uint32_t kSatietyDecaySeconds = 600;
    static constexpr uint32_t kCleanlinessDecaySeconds = 900;
    static constexpr uint32_t kMoodDecaySeconds = 1200;
    static constexpr uint8_t kLowNeedThreshold = 50;
#if defined(PET_GROWTH_TEST_MODE)
    static constexpr uint32_t kEggSmallCrackAgeMilliseconds = 10UL * 1000;
    static constexpr uint32_t kEggLargeCrackAgeMilliseconds = 12500;
    static constexpr uint32_t kEggHatchAgeSeconds = 15;
    static constexpr uint32_t kAdultAgeSeconds = 2UL * 60 + kEggHatchAgeSeconds;
#else
    static constexpr uint32_t kEggSmallCrackAgeMilliseconds = 4UL * 60 * 1000;
    static constexpr uint32_t kEggLargeCrackAgeMilliseconds = 270UL * 1000;
    static constexpr uint32_t kEggHatchAgeSeconds = 5UL * 60;
    static constexpr uint32_t kAdultAgeSeconds = 60UL * 60 + kEggHatchAgeSeconds;
#endif
    static constexpr uint32_t kBabySatietyDecaySeconds = 60;
    static constexpr uint32_t kBabyCleanlinessDecaySeconds = 90;
    static constexpr uint32_t kBabyMoodDecaySeconds = 120;
#if defined(PET_DEATH_TEST_MODE) || defined(PET_MEMORIAL_TEST_MODE)
    static constexpr uint32_t kSicknessExposureSeconds = 10;
    static constexpr uint32_t kDeathAfterSickAwakeSeconds = 30;
#elif defined(PET_SLEEP_TEST_MODE)
    static constexpr uint32_t kSicknessExposureSeconds = 20;
    static constexpr uint32_t kDeathAfterSickAwakeSeconds = 30;
#else
    static constexpr uint32_t kSicknessExposureSeconds = 6UL * 60 * 60;
    static constexpr uint32_t kDeathAfterSickAwakeSeconds = 24UL * 60 * 60;
#endif
    static constexpr uint8_t kFeedAmount = 20;
    static constexpr uint8_t kCleanAmount = 30;
    static constexpr uint8_t kPlayAmount = 15;
#if defined(PET_SLEEP_TEST_MODE) || defined(PET_DEEP_SLEEP_TEST_MODE)
    static constexpr uint32_t kSleepMoodRecoverySeconds = 10;
#else
    static constexpr uint32_t kSleepMoodRecoverySeconds = 20UL * 60;
#endif
    // Normal sleep earns 2 units/second, one low need earns 1, two earn 0.
    static constexpr uint32_t kSleepMoodRecoveryProgress = 2 * kSleepMoodRecoverySeconds;

    PetData();

    uint64_t petId() const;
    const char* name() const;
    SpeciesId speciesId() const;
    LifeStage lifeStage() const;
    HealthState healthState() const;
    uint8_t satiety() const;
    uint8_t mood() const;
    uint8_t cleanliness() const;
    uint8_t level() const;
    uint16_t exp() const;
    static constexpr uint8_t kMaxLevel = 20;
    uint16_t expToNextLevel() const;
    // Returns accepted EXP (limited by the remaining distance to max level).
    uint16_t gainExp(uint16_t amount);
    bool isSick() const;
    bool isDead() const;
    uint64_t ageSeconds() const;
    uint32_t displayRevision() const;
    uint32_t dangerSeconds() const;
    uint32_t sickAwakeSeconds() const;
    SleepMode sleepMode() const;
    PetSnapshotV1 snapshot() const;
    // Restores only validated snapshots and leaves the current pet untouched
    // on failure. Runtime-only display revision is never loaded from storage.
    bool restore(const PetSnapshotV1& snapshot);
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
    // Resets the current logical pet to the egg stage while retaining its
    // identity. Used by the dedicated growth test only.
    void startNewEgg();
    // Starts a distinct logical pet. Identity is validated before any state is
    // changed so a failed adoption cannot partially overwrite a dead pet.
    bool startNewEgg(uint64_t petId, const char* name);
    // Advances awake time in whole seconds. Keep the remainders with the pet
    // so future recovery can resume without losing partial intervals.
    void advanceSeconds(uint32_t seconds);
    // Normal sleep keeps needs, sickness and death progressing, but replaces
    // awake mood decay with slow mood recovery.
    void advanceSleepSeconds(uint32_t seconds);
    bool beginNormalSleep();
    bool beginDeepSleep();
    uint32_t sleepRecoveryProgress() const { return sleepMoodRecoveryProgress_; }
    bool restoreSleepRecoveryProgress(uint32_t progress);
    bool wake();
    bool feed();
    bool clean();
    bool play();
    bool treat();

private:
    uint16_t addExp(uint16_t amount);
    static uint8_t clampNeedValue(int64_t value);
    uint32_t advanceCareSeconds(uint32_t seconds, uint32_t satietyInterval,
                                uint32_t cleanlinessInterval,
                                uint32_t moodInterval, bool sleeping);
    void advanceSecondsForMode(uint32_t seconds, bool sleeping);
    void hatch();
    void growUpIfReady();

    uint64_t petId_;
    char name_[kPetNameMaxLength + 1];
    SpeciesId speciesId_;
    LifeStage lifeStage_;
    uint8_t satiety_;
    uint8_t mood_;
    uint8_t cleanliness_;
    uint8_t level_;
    uint16_t exp_;
    HealthState healthState_;
    uint64_t ageSeconds_;
    uint32_t satietyRemainderSeconds_;
    uint32_t cleanlinessRemainderSeconds_;
    uint32_t moodRemainderSeconds_;
    uint32_t dangerSeconds_;
    uint32_t sickAwakeSeconds_;
    SavedTimestamp bornAt_;
    SavedTimestamp diedAt_;
    DeathCause deathCause_;
    SleepMode sleepMode_;
    SavedTimestamp sleepStartedAt_;
    SavedTimestamp lastSleepSettledAt_;
    // Carries across sleep sessions while running and via the deep-sleep RTC
    // checkpoint in half-speed seconds (2 units per normal second). Paused
    // recovery retains earned progress. Cold resets clear it; NVS V1 is unchanged.
    uint32_t sleepMoodRecoveryProgress_;
    uint32_t displayRevision_;
};

}  // namespace Pet
