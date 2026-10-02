#include "pet/PetData.h"

#include <limits.h>
#include <string.h>

namespace Pet {

PetData::PetData()
    : petId_(1),
      speciesId_(SpeciesId::Bird),
      lifeStage_(LifeStage::Adult),
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
      sleepMoodRecoveryRemainderSeconds_(0),
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
uint16_t PetData::expToNextLevel() const {
    return level_ >= kMaxLevel ? 0 : 50 + 25 * (level_ - 1);
}
uint16_t PetData::gainExp(uint16_t amount) {
    if (!amount || isSick() || lifeStage_ == LifeStage::Egg ||
        sleepMode_ != SleepMode::Awake || level_ >= kMaxLevel) return 0;
    uint32_t remaining = 0;
    for (uint8_t lv = level_; lv < kMaxLevel; ++lv) remaining += 50 + 25 * (lv - 1);
    remaining -= exp_;
    const uint16_t accepted = amount > remaining ? remaining : amount;
    uint32_t progress = static_cast<uint32_t>(exp_) + accepted;
    while (level_ < kMaxLevel && progress >= expToNextLevel()) {
        progress -= expToNextLevel();
        ++level_;
    }
    exp_ = level_ == kMaxLevel ? 0 : static_cast<uint16_t>(progress);
    if (accepted) ++displayRevision_;
    return accepted;
}
bool PetData::isSick() const { return healthState_ != HealthState::Healthy; }
bool PetData::isDead() const { return healthState_ == HealthState::Dead; }
uint64_t PetData::ageSeconds() const { return ageSeconds_; }
uint32_t PetData::displayRevision() const { return displayRevision_; }
uint32_t PetData::dangerSeconds() const { return dangerSeconds_; }
uint32_t PetData::sickAwakeSeconds() const { return sickAwakeSeconds_; }
SleepMode PetData::sleepMode() const { return sleepMode_; }

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
    // Keep V1 files readable, including historical unrestricted setter values.
    level_ = saved.level > kMaxLevel ? kMaxLevel : saved.level;
    uint32_t progress = saved.exp;
    while (level_ < kMaxLevel && progress >= expToNextLevel()) {
        progress -= expToNextLevel();
        ++level_;
    }
    exp_ = level_ == kMaxLevel ? 0 : static_cast<uint16_t>(progress);
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
    sleepMoodRecoveryRemainderSeconds_ = 0;
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
    if (value > kMaxLevel) value = kMaxLevel;
    if (level_ != value) { level_ = value; ++displayRevision_; }
    setExp(exp_);
}
void PetData::setExp(uint16_t value) {
    const uint16_t bounded = level_ == kMaxLevel ? 0 :
        (value >= expToNextLevel() ? expToNextLevel() - 1 : value);
    if (exp_ != bounded) { exp_ = bounded; ++displayRevision_; }
}
void PetData::setSick(bool value) {
    if (isDead() || lifeStage_ == LifeStage::Egg) return;
    const HealthState next = value ? HealthState::Sick : HealthState::Healthy;
    if (healthState_ != next) { healthState_ = next; ++displayRevision_; }
    if (!value) { dangerSeconds_ = 0; sickAwakeSeconds_ = 0; }
}
void PetData::setDead(bool value) {
    if (lifeStage_ == LifeStage::Egg) return;
    if (isDead() && !value) return;
    if (value && !isDead()) {
        healthState_ = HealthState::Dead;
        deathCause_ = DeathCause::UntreatedSickness;
        sleepMode_ = SleepMode::Awake;
        sleepStartedAt_ = {0, false};
        lastSleepSettledAt_ = {0, false};
        sleepMoodRecoveryRemainderSeconds_ = 0;
        ++displayRevision_;
    }
}

void PetData::startNewEgg() {
    lifeStage_ = LifeStage::Egg;
    satiety_ = 80;
    mood_ = 80;
    cleanliness_ = 100;
    level_ = 1;
    exp_ = 0;
    healthState_ = HealthState::Healthy;
    ageSeconds_ = 0;
    satietyRemainderSeconds_ = 0;
    cleanlinessRemainderSeconds_ = 0;
    moodRemainderSeconds_ = 0;
    dangerSeconds_ = 0;
    sickAwakeSeconds_ = 0;
    bornAt_ = {0, false};
    diedAt_ = {0, false};
    deathCause_ = DeathCause::None;
    sleepMode_ = SleepMode::Awake;
    sleepStartedAt_ = {0, false};
    lastSleepSettledAt_ = {0, false};
    sleepMoodRecoveryRemainderSeconds_ = 0;
    ++displayRevision_;
}

bool PetData::startNewEgg(uint64_t petId, const char* name) {
    if (petId == 0 || !isValidPetName(name)) return false;
    petId_ = petId;
    memset(name_, 0, sizeof(name_));
    strncpy(name_, name, kPetNameMaxLength);
    speciesId_ = SpeciesId::Bird;
    startNewEgg();
    return true;
}

void PetData::hatch() {
    lifeStage_ = LifeStage::Baby;
    satiety_ = 80;
    mood_ = 80;
    cleanliness_ = 100;
    satietyRemainderSeconds_ = 0;
    cleanlinessRemainderSeconds_ = 0;
    moodRemainderSeconds_ = 0;
    dangerSeconds_ = 0;
    sickAwakeSeconds_ = 0;
    ++displayRevision_;
}

void PetData::growUpIfReady() {
    if (lifeStage_ != LifeStage::Baby || ageSeconds_ < kAdultAgeSeconds ||
        healthState_ != HealthState::Healthy) return;
    lifeStage_ = LifeStage::Adult;
    ++displayRevision_;
}

uint32_t PetData::advanceCareSeconds(uint32_t seconds,
                                     uint32_t satietyInterval,
                                     uint32_t cleanlinessInterval,
                                     uint32_t moodInterval,
                                     bool sleeping) {
    const uint64_t satietyZeroAt = satiety_ == 0 ? 0 :
        static_cast<uint64_t>(satiety_) * satietyInterval - satietyRemainderSeconds_;
    const uint64_t cleanlinessZeroAt = cleanliness_ == 0 ? 0 :
        static_cast<uint64_t>(cleanliness_) * cleanlinessInterval - cleanlinessRemainderSeconds_;
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
    satietyRemainderSeconds_ = satietyTotal % satietyInterval;
    cleanlinessRemainderSeconds_ = cleanlinessTotal % cleanlinessInterval;
    changeSatiety(-static_cast<int>(satietyTotal / satietyInterval));
    changeCleanliness(-static_cast<int>(cleanlinessTotal / cleanlinessInterval));
    if (sleeping) {
        const uint64_t moodRecoveryTotal =
            static_cast<uint64_t>(sleepMoodRecoveryRemainderSeconds_) +
            elapsedSeconds;
        sleepMoodRecoveryRemainderSeconds_ = static_cast<uint32_t>(
            moodRecoveryTotal % kSleepMoodRecoverySeconds);
        changeMood(static_cast<int>(moodRecoveryTotal /
                                    kSleepMoodRecoverySeconds));
    } else {
        const uint64_t moodTotal =
            static_cast<uint64_t>(moodRemainderSeconds_) + elapsedSeconds;
        moodRemainderSeconds_ = moodTotal % moodInterval;
        changeMood(-static_cast<int>(moodTotal / moodInterval));
    }
    if (diesThisAdvance) setDead(true);
    return elapsedSeconds;
}

void PetData::advanceSecondsForMode(uint32_t seconds, bool sleeping) {
    if (isDead() || seconds == 0) return;
    uint32_t remaining = seconds;
    while (remaining > 0 && !isDead()) {
        if (lifeStage_ == LifeStage::Egg) {
            const uint32_t untilHatch = static_cast<uint32_t>(
                kEggHatchAgeSeconds - ageSeconds_);
            const uint32_t elapsed = remaining < untilHatch ? remaining : untilHatch;
            ageSeconds_ += elapsed;
            remaining -= elapsed;
            if (ageSeconds_ >= kEggHatchAgeSeconds) hatch();
            continue;
        }

        uint32_t elapsed = remaining;
        if (lifeStage_ == LifeStage::Baby && ageSeconds_ < kAdultAgeSeconds) {
            const uint32_t untilAdult = static_cast<uint32_t>(kAdultAgeSeconds - ageSeconds_);
            if (elapsed > untilAdult) elapsed = untilAdult;
        }
        const bool baby = lifeStage_ == LifeStage::Baby;
        const uint32_t consumed = advanceCareSeconds(
            elapsed,
            baby ? kBabySatietyDecaySeconds : kSatietyDecaySeconds,
            baby ? kBabyCleanlinessDecaySeconds : kCleanlinessDecaySeconds,
            baby ? kBabyMoodDecaySeconds : kMoodDecaySeconds,
            sleeping);
        remaining -= consumed;
        if (isDead() || consumed < elapsed) return;
        growUpIfReady();
    }
}

void PetData::advanceSeconds(uint32_t seconds) {
    if (sleepMode_ != SleepMode::Awake) return;
    advanceSecondsForMode(seconds, false);
}

void PetData::advanceSleepSeconds(uint32_t seconds) {
    if (sleepMode_ != SleepMode::Normal && sleepMode_ != SleepMode::Deep) return;
    advanceSecondsForMode(seconds, true);
}

bool PetData::beginNormalSleep() {
    if (isDead() || lifeStage_ == LifeStage::Egg ||
        sleepMode_ != SleepMode::Awake) return false;
    sleepMode_ = SleepMode::Normal;
    sleepStartedAt_ = {0, false};
    lastSleepSettledAt_ = {0, false};
    moodRemainderSeconds_ = 0;
    ++displayRevision_;
    return true;
}

bool PetData::wake() {
    if (isDead() || sleepMode_ == SleepMode::Awake) return false;
    sleepMode_ = SleepMode::Awake;
    sleepStartedAt_ = {0, false};
    lastSleepSettledAt_ = {0, false};
    growUpIfReady();
    ++displayRevision_;
    return true;
}

bool PetData::beginDeepSleep() {
    if (!beginNormalSleep()) return false;
    sleepMode_ = SleepMode::Deep;
    return true;
}

bool PetData::restoreSleepRecoveryRemainder(uint32_t seconds) {
    if (seconds >= kSleepMoodRecoverySeconds || sleepMode_ == SleepMode::Awake) return false;
    sleepMoodRecoveryRemainderSeconds_ = seconds;
    return true;
}

bool PetData::feed() {
    if (isDead() || lifeStage_ == LifeStage::Egg) return false;
    changeSatiety(kFeedAmount);
    return true;
}

bool PetData::clean() {
    if (isDead() || lifeStage_ == LifeStage::Egg) return false;
    changeCleanliness(kCleanAmount);
    return true;
}

bool PetData::play() {
    if (isDead() || lifeStage_ == LifeStage::Egg) return false;
    changeMood(kPlayAmount);
    return true;
}

bool PetData::treat() {
    if (isDead() || lifeStage_ == LifeStage::Egg || !isSick()) return false;
    setSick(false);
    growUpIfReady();
    return true;
}

uint8_t PetData::clampNeedValue(int64_t value) {
    if (value < kMinNeedValue) return kMinNeedValue;
    if (value > kMaxNeedValue) return kMaxNeedValue;
    return static_cast<uint8_t>(value);
}

}  // namespace Pet
