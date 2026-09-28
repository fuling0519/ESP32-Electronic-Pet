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
    return stage == LifeStage::Egg || stage == LifeStage::Baby ||
           stage == LifeStage::Adult;
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
    const uint32_t satietyInterval = s.lifeStage == LifeStage::Baby ?
        PetData::kBabySatietyDecaySeconds : PetData::kSatietyDecaySeconds;
    const uint32_t cleanlinessInterval = s.lifeStage == LifeStage::Baby ?
        PetData::kBabyCleanlinessDecaySeconds : PetData::kCleanlinessDecaySeconds;
    const uint32_t moodInterval = s.lifeStage == LifeStage::Baby ?
        PetData::kBabyMoodDecaySeconds : PetData::kMoodDecaySeconds;
    if (s.satietyRemainderSeconds >= satietyInterval ||
        s.cleanlinessRemainderSeconds >= cleanlinessInterval ||
        s.moodRemainderSeconds >= moodInterval ||
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
        (s.healthState != HealthState::Healthy ||
         s.sleepMode != SleepMode::Awake ||
         s.ageSeconds >= PetData::kEggHatchAgeSeconds ||
         s.satietyRemainderSeconds != 0 ||
         s.cleanlinessRemainderSeconds != 0 ||
         s.moodRemainderSeconds != 0 || s.dangerSeconds != 0 ||
         s.sickAwakeSeconds != 0)) return false;
    if (s.lifeStage == LifeStage::Baby &&
        s.ageSeconds < PetData::kEggHatchAgeSeconds) return false;
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
