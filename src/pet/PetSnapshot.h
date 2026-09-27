#pragma once

#include <stdint.h>

namespace Pet {

constexpr uint8_t kPetNameMaxLength = 8;

enum class SpeciesId : uint8_t {
    Bird = 1,
};

enum class LifeStage : uint8_t {
    Egg = 0,
    Hatched = 1,
};

enum class HealthState : uint8_t {
    Healthy = 0,
    Sick = 1,
    Dead = 2,
};

enum class SleepMode : uint8_t {
    Awake = 0,
    Normal = 1,
    Deep = 2,
};

enum class DeathCause : uint8_t {
    None = 0,
    UntreatedSickness = 1,
};

// A calendar timestamp is meaningful only when valid is true. Invalid
// timestamps must keep unixSeconds at zero so a later clock setup cannot make
// an unknown historical time appear trustworthy.
struct SavedTimestamp {
    uint64_t unixSeconds;
    bool valid;
};

// Complete logical state needed to resume one pet. This is deliberately
// separate from the byte-level NVS encoding; never persist sizeof(this struct)
// because compiler padding and enum representation are not a stable format.
struct PetSnapshotV1 {
    uint64_t petId;
    char name[kPetNameMaxLength + 1];
    SpeciesId speciesId;
    LifeStage lifeStage;

    uint8_t satiety;
    uint8_t mood;
    uint8_t cleanliness;
    uint8_t level;
    uint16_t exp;
    HealthState healthState;
    uint64_t ageSeconds;

    uint32_t satietyRemainderSeconds;
    uint32_t cleanlinessRemainderSeconds;
    uint32_t moodRemainderSeconds;
    uint32_t dangerSeconds;
    uint32_t sickAwakeSeconds;

    SavedTimestamp bornAt;
    SavedTimestamp diedAt;
    DeathCause deathCause;

    SleepMode sleepMode;
    SavedTimestamp sleepStartedAt;
    SavedTimestamp lastSleepSettledAt;
};

bool isValidPetName(const char* name);
bool isValidPetSnapshot(const PetSnapshotV1& snapshot);

}  // namespace Pet
