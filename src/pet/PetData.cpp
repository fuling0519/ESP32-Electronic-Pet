#include "pet/PetData.h"

#include <limits.h>
#include <string.h>

namespace Pet {

PetData::PetData()
    : petId_(1),
      speciesId_(SpeciesId::Bird),
      lifeStage_(LifeStage::Hatched),
      satiety_(80),
      mood_(80),
      cleanliness_(100),
      level_(1),
      exp_(0),
      healthState_(HealthState::Healthy),
      ageSeconds_(0),
      satietyRemainderSeconds_(0),
      cleanlinessRemainderSeconds_(0),
      moodRemainderSeconds_(0),
      dangerSeconds_(0),
      sickAwakeSeconds_(0),
      bornAt_{0, false},
      diedAt_{0, false},
      deathCause_(DeathCause::None),
      sleepMode_(SleepMode::Awake),
      sleepStartedAt_{0, false},
      lastSleepSettledAt_{0, false},
      displayRevision_(0) {
    memcpy(name_, "Pet-0001", sizeof(name_));
}

uint64_t PetData::petId() const { return petId_; }
const char* PetData::name() const { return name_; }
SpeciesId PetData::speciesId() const { return speciesId_; }
LifeStage PetData::lifeStage() const { return lifeStage_; }
HealthState PetData::healthState() const { return healthState_; }
uint8_t PetData::satiety() const { return satiety_; }
uint8_t PetData::mood() const { return mood_; }
uint8_t PetData::cleanliness() const { return cleanliness_; }
uint8_t PetData::level() const { return level_; }
uint16_t PetData::exp() const { return exp_; }
bool PetData::isSick() const { return healthState_ != HealthState::Healthy; }
bool PetData::isDead() const { return healthState_ == HealthState::Dead; }
uint64_t PetData::ageSeconds() const { return ageSeconds_; }
uint32_t PetData::displayRevision() const { return displayRevision_; }
uint32_t PetData::dangerSeconds() const { return dangerSeconds_; }
uint32_t PetData::sickAwakeSeconds() const { return sickAwakeSeconds_; }

PetSnapshotV1 PetData::snapshot() const {
    PetSnapshotV1 result{};
    result.petId = petId_;
    memcpy(result.name, name_, sizeof(result.name));
    result.speciesId = speciesId_;
    result.lifeStage = lifeStage_;
    result.satiety = satiety_;
    result.mood = mood_;
    result.cleanliness = cleanliness_;
    result.level = level_;
    result.exp = exp_;
    result.healthState = healthState_;
    result.ageSeconds = ageSeconds_;
    result.satietyRemainderSeconds = satietyRemainderSeconds_;
    result.cleanlinessRemainderSeconds = cleanlinessRemainderSeconds_;
    result.moodRemainderSeconds = moodRemainderSeconds_;
    result.dangerSeconds = dangerSeconds_;
    result.sickAwakeSeconds = sickAwakeSeconds_;
    result.bornAt = bornAt_;
    result.diedAt = diedAt_;
    result.deathCause = deathCause_;
    result.sleepMode = sleepMode_;
    result.sleepStartedAt = sleepStartedAt_;
    result.lastSleepSettledAt = lastSleepSettledAt_;
    return result;
}

bool PetData::restore(const PetSnapshotV1& saved) {
    if (!isValidPetSnapshot(saved)) return false;
    petId_ = saved.petId;
    memcpy(name_, saved.name, sizeof(name_));
    speciesId_ = saved.speciesId;
    lifeStage_ = saved.lifeStage;
    satiety_ = saved.satiety;
    mood_ = saved.mood;
    cleanliness_ = saved.cleanliness;
    level_ = saved.level;
    exp_ = saved.exp;
    healthState_ = saved.healthState;
    ageSeconds_ = saved.ageSeconds;
    satietyRemainderSeconds_ = saved.satietyRemainderSeconds;
    cleanlinessRemainderSeconds_ = saved.cleanlinessRemainderSeconds;
    moodRemainderSeconds_ = saved.moodRemainderSeconds;
    dangerSeconds_ = saved.dangerSeconds;
    sickAwakeSeconds_ = saved.sickAwakeSeconds;
    bornAt_ = saved.bornAt;
    diedAt_ = saved.diedAt;
    deathCause_ = saved.deathCause;
    sleepMode_ = saved.sleepMode;
    sleepStartedAt_ = saved.sleepStartedAt;
    lastSleepSettledAt_ = saved.lastSleepSettledAt;
    ++displayRevision_;
    return true;
}

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

void PetData::setSatiety(int value) {
    const uint8_t bounded = clampNeedValue(value);
    if (satiety_ != bounded) { satiety_ = bounded; ++displayRevision_; }
    if (satiety_ > 0 && cleanliness_ > 0 && !isSick()) dangerSeconds_ = 0;
}
void PetData::setMood(int value) {
    const uint8_t bounded = clampNeedValue(value);
    if (mood_ != bounded) { mood_ = bounded; ++displayRevision_; }
}
void PetData::setCleanliness(int value) {
    const uint8_t bounded = clampNeedValue(value);
    if (cleanliness_ != bounded) { cleanliness_ = bounded; ++displayRevision_; }
    if (satiety_ > 0 && cleanliness_ > 0 && !isSick()) dangerSeconds_ = 0;
}

void PetData::changeSatiety(int amount) {
    setSatiety(clampNeedValue(static_cast<int64_t>(satiety_) + amount));
}
void PetData::changeMood(int amount) {
    setMood(clampNeedValue(static_cast<int64_t>(mood_) + amount));
}
void PetData::changeCleanliness(int amount) {
    setCleanliness(clampNeedValue(static_cast<int64_t>(cleanliness_) + amount));
}

void PetData::setLevel(uint8_t value) {
    if (value == 0) value = 1;
    if (level_ != value) { level_ = value; ++displayRevision_; }
}
void PetData::setExp(uint16_t value) { exp_ = value; }
void PetData::setSick(bool value) {
    if (isDead()) return;
    const HealthState next = value ? HealthState::Sick : HealthState::Healthy;
    if (healthState_ != next) { healthState_ = next; ++displayRevision_; }
    if (!value) { dangerSeconds_ = 0; sickAwakeSeconds_ = 0; }
}
void PetData::setDead(bool value) {
    if (isDead() && !value) return;
    if (value && !isDead()) {
        healthState_ = HealthState::Dead;
        deathCause_ = DeathCause::UntreatedSickness;
        ++displayRevision_;
    }
}

void PetData::advanceSeconds(uint32_t seconds) {
    if (isDead() || seconds == 0) return;
    const uint64_t satietyZeroAt = satiety_ == 0 ? 0 :
        static_cast<uint64_t>(satiety_) * kSatietyDecaySeconds - satietyRemainderSeconds_;
    const uint64_t cleanlinessZeroAt = cleanliness_ == 0 ? 0 :
        static_cast<uint64_t>(cleanliness_) * kCleanlinessDecaySeconds - cleanlinessRemainderSeconds_;
    const uint64_t dangerStartsAt = satietyZeroAt < cleanlinessZeroAt ?
        satietyZeroAt : cleanlinessZeroAt;
    const uint64_t exposedSeconds = seconds > dangerStartsAt ? seconds - dangerStartsAt : 0;
    uint32_t elapsedSeconds = seconds;
    bool diesThisAdvance = false;
    if (isSick()) {
        const uint32_t remaining = kDeathAfterSickAwakeSeconds - sickAwakeSeconds_;
        if (seconds >= remaining) {
            elapsedSeconds = remaining;
            diesThisAdvance = true;
        }
    } else if (exposedSeconds >= kSicknessExposureSeconds - dangerSeconds_) {
        const uint64_t deathAt = dangerStartsAt +
            (kSicknessExposureSeconds - dangerSeconds_) +
            kDeathAfterSickAwakeSeconds;
        if (seconds >= deathAt) {
            elapsedSeconds = static_cast<uint32_t>(deathAt);
            diesThisAdvance = true;
        }
    }
    const uint64_t effectiveExposedSeconds = elapsedSeconds > dangerStartsAt ?
        elapsedSeconds - dangerStartsAt : 0;
    if (isSick()) {
        const uint64_t total = static_cast<uint64_t>(sickAwakeSeconds_) + elapsedSeconds;
        sickAwakeSeconds_ = total > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(total);
    } else if (effectiveExposedSeconds > 0) {
        const uint32_t remaining = kSicknessExposureSeconds - dangerSeconds_;
        if (effectiveExposedSeconds >= remaining) {
            setSick(true);
            dangerSeconds_ = kSicknessExposureSeconds;
            sickAwakeSeconds_ = effectiveExposedSeconds - remaining > UINT32_MAX ? UINT32_MAX :
                static_cast<uint32_t>(effectiveExposedSeconds - remaining);
        } else {
            dangerSeconds_ += static_cast<uint32_t>(effectiveExposedSeconds);
        }
    }
    if (UINT64_MAX - ageSeconds_ < elapsedSeconds) ageSeconds_ = UINT64_MAX;
    else ageSeconds_ += elapsedSeconds;

    const uint64_t satietyTotal = static_cast<uint64_t>(satietyRemainderSeconds_) + elapsedSeconds;
    const uint64_t cleanlinessTotal = static_cast<uint64_t>(cleanlinessRemainderSeconds_) + elapsedSeconds;
    const uint64_t moodTotal = static_cast<uint64_t>(moodRemainderSeconds_) + elapsedSeconds;
    satietyRemainderSeconds_ = satietyTotal % kSatietyDecaySeconds;
    cleanlinessRemainderSeconds_ = cleanlinessTotal % kCleanlinessDecaySeconds;
    moodRemainderSeconds_ = moodTotal % kMoodDecaySeconds;
    changeSatiety(-static_cast<int>(satietyTotal / kSatietyDecaySeconds));
    changeCleanliness(-static_cast<int>(cleanlinessTotal / kCleanlinessDecaySeconds));
    changeMood(-static_cast<int>(moodTotal / kMoodDecaySeconds));
    if (diesThisAdvance) setDead(true);
}

bool PetData::feed() {
    if (isDead()) return false;
    changeSatiety(kFeedAmount);
    return true;
}

bool PetData::clean() {
    if (isDead()) return false;
    changeCleanliness(kCleanAmount);
    return true;
}

bool PetData::treat() {
    if (isDead() || !isSick()) return false;
    setSick(false);
    return true;
}

uint8_t PetData::clampNeedValue(int64_t value) {
    if (value < kMinNeedValue) return kMinNeedValue;
    if (value > kMaxNeedValue) return kMaxNeedValue;
    return static_cast<uint8_t>(value);
}

}  // namespace Pet
