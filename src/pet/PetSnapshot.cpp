#include "pet/PetSnapshot.h"

#include "pet/PetData.h"

namespace Pet {
namespace {

bool isValidTimestamp(const SavedTimestamp& timestamp) {
    return timestamp.valid ? timestamp.unixSeconds > 0 : timestamp.unixSeconds == 0;
}

bool isKnownSpecies(SpeciesId species) {
    return species == SpeciesId::Bird;
}

bool isKnownLifeStage(LifeStage stage) {
    return stage == LifeStage::Egg || stage == LifeStage::Hatched;
}

bool isKnownHealthState(HealthState state) {
    return state == HealthState::Healthy || state == HealthState::Sick ||
           state == HealthState::Dead;
}

bool isKnownSleepMode(SleepMode mode) {
    return mode == SleepMode::Awake || mode == SleepMode::Normal ||
           mode == SleepMode::Deep;
}

bool isKnownDeathCause(DeathCause cause) {
    return cause == DeathCause::None || cause == DeathCause::UntreatedSickness;
}

}  // namespace

bool isValidPetName(const char* name) {
    if (name == nullptr) return false;
    uint8_t length = 0;
    while (length < kPetNameMaxLength + 1 && name[length] != '\0') {
        const uint8_t character = static_cast<uint8_t>(name[length]);
        if (character < 0x20 || character > 0x7E) return false;
        ++length;
    }
    return length >= 1 && length <= kPetNameMaxLength;
}

bool isValidPetSnapshot(const PetSnapshotV1& s) {
    if (s.petId == 0 || !isValidPetName(s.name) || !isKnownSpecies(s.speciesId) ||
        !isKnownLifeStage(s.lifeStage) || !isKnownHealthState(s.healthState) ||
        !isKnownSleepMode(s.sleepMode) || !isKnownDeathCause(s.deathCause)) {
        return false;
    }
    if (s.satiety > PetData::kMaxNeedValue || s.mood > PetData::kMaxNeedValue ||
        s.cleanliness > PetData::kMaxNeedValue || s.level == 0) {
        return false;
    }
    if (s.satietyRemainderSeconds >= PetData::kSatietyDecaySeconds ||
        s.cleanlinessRemainderSeconds >= PetData::kCleanlinessDecaySeconds ||
        s.moodRemainderSeconds >= PetData::kMoodDecaySeconds ||
        s.dangerSeconds > PetData::kSicknessExposureSeconds ||
        s.sickAwakeSeconds > PetData::kDeathAfterSickAwakeSeconds) {
        return false;
    }
    if (!isValidTimestamp(s.bornAt) || !isValidTimestamp(s.diedAt) ||
        !isValidTimestamp(s.sleepStartedAt) ||
        !isValidTimestamp(s.lastSleepSettledAt)) {
        return false;
    }
    if (s.bornAt.valid && s.diedAt.valid && s.diedAt.unixSeconds < s.bornAt.unixSeconds) {
        return false;
    }
    if (s.lifeStage == LifeStage::Egg &&
        (s.healthState != HealthState::Healthy || s.ageSeconds != 0)) {
        return false;
    }
    if (s.healthState == HealthState::Healthy) {
        if (s.sickAwakeSeconds != 0 ||
            s.dangerSeconds >= PetData::kSicknessExposureSeconds) {
            return false;
        }
        if (s.satiety > 0 && s.cleanliness > 0 && s.dangerSeconds != 0) return false;
    }
    if (s.healthState == HealthState::Sick &&
        s.sickAwakeSeconds >= PetData::kDeathAfterSickAwakeSeconds) {
        return false;
    }
    if (s.healthState == HealthState::Dead) {
        if (s.deathCause == DeathCause::None || s.sleepMode != SleepMode::Awake) return false;
    } else if (s.deathCause != DeathCause::None || s.diedAt.valid) {
        return false;
    }
    if (s.sleepMode == SleepMode::Awake) {
        if (s.sleepStartedAt.valid || s.lastSleepSettledAt.valid) return false;
    } else {
        if (s.sleepStartedAt.valid != s.lastSleepSettledAt.valid) return false;
        if (s.sleepStartedAt.valid &&
            s.lastSleepSettledAt.unixSeconds < s.sleepStartedAt.unixSeconds) {
            return false;
        }
    }
    return true;
}

}  // namespace Pet
