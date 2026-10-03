#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "pet/PetClock.h"
#include "pet/PetData.h"
#include "pet/PetName.h"
#include "storage/Memorials.h"
#include "power/SleepSettlement.h"

int main() {
    // Care rewards depend on actual benefit, including care while sick.
    Pet::PetData cared;
    cared.setSatiety(99); cared.setCleanliness(99);
    assert(cared.feed() && cared.exp() == 3 && cared.satiety() == 100);
    assert(cared.feed() && cared.exp() == 3);
    assert(cared.clean() && cared.exp() == 6 && cared.cleanliness() == 100);
    assert(cared.clean() && cared.exp() == 6);
    assert(!cared.treat() && cared.exp() == 6);
    cared.setSick(true); cared.setSatiety(50); cared.setCleanliness(50);
    assert(cared.feed() && cared.exp() == 9);
    assert(cared.clean() && cared.exp() == 12);
    assert(cared.treat() && cared.exp() == 17 && !cared.isSick());
    assert(!cared.treat() && cared.exp() == 17);
    cared.setExp(49); cared.setSatiety(99);
    assert(cared.feed() && cared.level() == 2 && cared.exp() == 2);
    Pet::PetData careRestored;
    assert(careRestored.restore(cared.snapshot()) && careRestored.exp() == 2);
    cared.setLevel(Pet::PetData::kMaxLevel); cared.setCleanliness(0);
    assert(cared.clean() && cared.cleanliness() == 30 && cared.exp() == 0);
    assert(cared.beginNormalSleep());
    assert(!cared.feed() && !cared.clean() && !cared.treat() && cared.exp() == 0);
    cared.wake(); cared.startNewEgg();
    assert(!cared.feed() && !cared.clean() && !cared.treat() && cared.exp() == 0);
    Pet::PetData experience;
    experience.setMood(100);
    experience.setExp(45);
    const auto expRevision = experience.displayRevision();
    assert(experience.gainExp(15) == 15);
    assert(experience.level() == 2 && experience.exp() == 10);
    assert(experience.expToNextLevel() == 75 && experience.mood() == 100);
    assert(experience.displayRevision() != expRevision);
    assert(experience.gainExp(200) == 200);
    assert(experience.level() == 4 && experience.exp() == 35);
    Pet::PetData expRestored;
    assert(expRestored.restore(experience.snapshot()));
    assert(expRestored.level() == 4 && expRestored.exp() == 35);
    assert(expRestored.beginDeepSleep());
    expRestored.advanceSleepSeconds(600);
    assert(expRestored.gainExp(15) == 0 && expRestored.exp() == 35);
    assert(expRestored.wake());
    expRestored.setSick(true);
    assert(expRestored.gainExp(15) == 0);
    expRestored.setDead(true);
    assert(expRestored.gainExp(15) == 0);
    assert(experience.startNewEgg(2, "Tamama"));
    assert(experience.level() == 1 && experience.exp() == 0 && experience.gainExp(15) == 0);
    experience.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
    assert(experience.gainExp(50) == 50 && experience.level() == 2);
    experience.setLevel(19); experience.setExp(495);
    assert(experience.gainExp(UINT16_MAX) == 5);
    assert(experience.level() == 20 && experience.exp() == 0 && experience.expToNextLevel() == 0);
    const auto maxRevision = experience.displayRevision();
    assert(experience.gainExp(15) == 0 && experience.displayRevision() == maxRevision);
    auto legacy = experience.snapshot(); legacy.level = 255; legacy.exp = UINT16_MAX;
    assert(expRestored.restore(legacy) && expRestored.level() == 20 && expRestored.exp() == 0);
    legacy.level = 1; legacy.exp = 125;
    assert(expRestored.restore(legacy) && expRestored.level() == 3 && expRestored.exp() == 0);
    Pet::PetData hugeReward;
    assert(hugeReward.gainExp(UINT16_MAX) == 5225 && hugeReward.level() == 20);
    uint32_t settledSeconds = 0, settledRemainderMs = 0;
    const uint32_t cap = 48UL * 60 * 60;
    assert(Power::sleepSettlement(1000000LL, 62500000LL, cap, settledSeconds, settledRemainderMs));
    assert(settledSeconds == 61 && settledRemainderMs == 500);
    assert(Power::sleepSettlement(0, static_cast<int64_t>(cap) * 1000000LL, cap, settledSeconds, settledRemainderMs));
    assert(settledSeconds == cap && settledRemainderMs == 0);
    assert(Power::sleepSettlement(0, 50LL * 60 * 60 * 1000000LL + 999000, cap, settledSeconds, settledRemainderMs));
    assert(settledSeconds == cap && settledRemainderMs == 0);
    assert(!Power::sleepSettlement(1000000LL, 999999LL, cap, settledSeconds, settledRemainderMs));
    assert(settledSeconds == 0 && settledRemainderMs == 0);
    assert(!Power::sleepSettlement(-1, 1000000LL, cap, settledSeconds, settledRemainderMs));
    Pet::PetData pet;
    assert(pet.ageSeconds() == 0);
    pet.advanceSeconds(599);
    assert(pet.satiety() == 80 && pet.ageSeconds() == 599);
    const uint32_t initialRevision = pet.displayRevision();
    pet.advanceSeconds(1);
    assert(pet.satiety() == 79 && pet.displayRevision() != initialRevision);
    pet.advanceSeconds(300);
    assert(pet.cleanliness() == 99 && pet.satiety() == 79);
    pet.advanceSeconds(300);
    assert(pet.satiety() == 78 && pet.mood() == 79);

    Pet::PetData batch;
    batch.advanceSeconds(1200);
    assert(batch.satiety() == pet.satiety());
    assert(batch.cleanliness() == pet.cleanliness());
    assert(batch.mood() == pet.mood());
    assert(batch.ageSeconds() == pet.ageSeconds());

    pet.changeSatiety(INT_MIN);
    pet.changeMood(INT_MAX);
    pet.changeCleanliness(INT_MIN);
    assert(pet.satiety() == 0 && pet.mood() == 100 && pet.cleanliness() == 0);
    pet.setLevel(0);
    assert(pet.level() == 1);
    pet.setSatiety(76);
    assert(pet.hungerState() == Pet::HungerState::Satisfied);
    pet.setSatiety(75);
    assert(pet.hungerState() == Pet::HungerState::SlightlyHungry);
    pet.setCleanliness(25);
    assert(pet.cleanlinessState() == Pet::CleanlinessState::Filthy);
    pet.setMood(20);
    assert(pet.moodState() == Pet::MoodState::VerySad);

    const uint64_t ageBeforeDeath = pet.ageSeconds();
    pet.setDead(true);
    pet.advanceSeconds(86400);
    assert(pet.ageSeconds() == ageBeforeDeath);
    assert(!pet.feed() && !pet.clean() && !pet.treat());

    Pet::PetData neglected;
    neglected.setSatiety(0);
    neglected.advanceSeconds(Pet::PetData::kSicknessExposureSeconds - 1);
    assert(!neglected.isSick());
    assert(neglected.dangerSeconds() == Pet::PetData::kSicknessExposureSeconds - 1);
    neglected.advanceSeconds(1);
    assert(neglected.isSick() && neglected.sickAwakeSeconds() == 0);
    neglected.advanceSeconds(60);
    assert(neglected.sickAwakeSeconds() == 60);
    assert(neglected.treat());
    assert(!neglected.isSick() && neglected.dangerSeconds() == 0);
    neglected.advanceSeconds(Pet::PetData::kSicknessExposureSeconds);
    assert(neglected.isSick());

    Pet::PetData rescued;
    rescued.setSatiety(0);
    rescued.advanceSeconds(1000);
    assert(rescued.dangerSeconds() == 1000);
    assert(rescued.feed());
    assert(rescued.dangerSeconds() == 0);
    rescued.setCleanliness(0);
    rescued.advanceSeconds(500);
    assert(rescued.dangerSeconds() == 500);
    assert(rescued.clean());
    assert(rescued.dangerSeconds() == 0);

    Pet::PetData exact;
    exact.advanceSeconds(80 * Pet::PetData::kSatietyDecaySeconds);
    assert(exact.satiety() == 0 && exact.dangerSeconds() == 0);
    exact.advanceSeconds(Pet::PetData::kSicknessExposureSeconds);
    assert(exact.isSick() && exact.sickAwakeSeconds() == 0);

    Pet::PetData batchedSickness;
    batchedSickness.advanceSeconds(80 * Pet::PetData::kSatietyDecaySeconds +
                                   Pet::PetData::kSicknessExposureSeconds + 90);
    assert(batchedSickness.isSick() && batchedSickness.sickAwakeSeconds() == 90);
    assert(batchedSickness.ageSeconds() == exact.ageSeconds() + 90);

    Pet::PetData dying;
    dying.setSick(true);
    dying.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds - 1);
    assert(dying.isSick() && !dying.isDead());
    const uint64_t ageBeforeFatalSecond = dying.ageSeconds();
    dying.advanceSeconds(1);
    assert(dying.isDead());
    assert(dying.sickAwakeSeconds() == Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(dying.ageSeconds() == ageBeforeFatalSecond + 1);
    dying.setDead(false);
    dying.setSick(false);
    assert(dying.isDead() && dying.isSick());

    Pet::PetData fatalBatch;
    fatalBatch.setSatiety(0);
    const uint32_t fatalElapsed = Pet::PetData::kSicknessExposureSeconds +
                                  Pet::PetData::kDeathAfterSickAwakeSeconds;
    fatalBatch.advanceSeconds(fatalElapsed + 3600);
    assert(fatalBatch.isDead());
    assert(fatalBatch.sickAwakeSeconds() == Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(fatalBatch.ageSeconds() == fatalElapsed);

    Pet::PetData sleeping;
    sleeping.setMood(50);
    sleeping.advanceSeconds(Pet::PetData::kMoodDecaySeconds - 1);
    assert(sleeping.mood() == 50);
    assert(sleeping.beginNormalSleep());
    assert(sleeping.sleepMode() == Pet::SleepMode::Normal);
    assert(Pet::isValidPetSnapshot(sleeping.snapshot()));
    const uint64_t sleepStartAge = sleeping.ageSeconds();
    sleeping.advanceSeconds(60);  // Awake updates are ignored while asleep.
    assert(sleeping.ageSeconds() == sleepStartAge);
    sleeping.advanceSleepSeconds(Pet::PetData::kSleepMoodRecoverySeconds - 1);
    assert(sleeping.mood() == 50);
    sleeping.advanceSleepSeconds(1);
    assert(sleeping.mood() == 51);
    assert(sleeping.satiety() == 77);
    assert(sleeping.wake());
    assert(sleeping.sleepMode() == Pet::SleepMode::Awake);
    assert(Pet::isValidPetSnapshot(sleeping.snapshot()));
    sleeping.advanceSeconds(Pet::PetData::kMoodDecaySeconds - 1);
    assert(sleeping.mood() == 51);
    sleeping.advanceSeconds(1);
    assert(sleeping.mood() == 50);

    Pet::PetData sleepingBatch;
    sleepingBatch.setMood(50);
    assert(sleepingBatch.beginNormalSleep());
    sleepingBatch.advanceSleepSeconds(Pet::PetData::kSleepMoodRecoverySeconds * 2);
    assert(sleepingBatch.mood() == 52);
    assert(sleepingBatch.satiety() == 76);
    assert(sleepingBatch.ageSeconds() ==
           Pet::PetData::kSleepMoodRecoverySeconds * 2);

    Pet::PetData sleepingNeglect;
    sleepingNeglect.setSatiety(0);
    assert(sleepingNeglect.beginNormalSleep());
    sleepingNeglect.advanceSleepSeconds(Pet::PetData::kSicknessExposureSeconds);
    assert(sleepingNeglect.isSick());
    assert(sleepingNeglect.sickAwakeSeconds() == 0);
    sleepingNeglect.advanceSleepSeconds(
        Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(sleepingNeglect.isDead());
    assert(sleepingNeglect.sleepMode() == Pet::SleepMode::Awake);
    assert(sleepingNeglect.sickAwakeSeconds() ==
           Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(sleepingNeglect.ageSeconds() ==
           static_cast<uint64_t>(Pet::PetData::kSicknessExposureSeconds) +
           Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(Pet::isValidPetSnapshot(sleepingNeglect.snapshot()));
    assert(!sleepingNeglect.wake());

    Pet::PetData manualDeath;
    manualDeath.setDead(true);
    assert(manualDeath.isDead() && manualDeath.isSick());
    assert(!manualDeath.treat());

    // A snapshot preserves partial decay and disease timers, while invalid
    // input is rejected without partially replacing the current pet.
    Pet::PetData beforeSave;
    beforeSave.advanceSeconds(599);
    beforeSave.setCleanliness(0);
    beforeSave.advanceSeconds(321);
    const Pet::PetSnapshotV1 saved = beforeSave.snapshot();
    assert(Pet::isValidPetSnapshot(saved));
    assert(saved.petId == 1 && saved.speciesId == Pet::SpeciesId::Bird);
    assert(saved.satietyRemainderSeconds == 320);
    assert(saved.dangerSeconds == 321);

    Pet::PetData restored;
    assert(restored.restore(saved));
    assert(restored.ageSeconds() == beforeSave.ageSeconds());
    assert(restored.dangerSeconds() == beforeSave.dangerSeconds());
    beforeSave.advanceSeconds(280);
    restored.advanceSeconds(280);
    assert(restored.satiety() == beforeSave.satiety());
    assert(restored.snapshot().satietyRemainderSeconds ==
           beforeSave.snapshot().satietyRemainderSeconds);

    Pet::PetSnapshotV1 invalid = saved;
    invalid.satiety = 101;
    const uint64_t ageBeforeInvalidRestore = restored.ageSeconds();
    assert(!restored.restore(invalid));
    assert(restored.ageSeconds() == ageBeforeInvalidRestore);

    Pet::PetData deadRoundTrip;
    deadRoundTrip.setDead(true);
    const Pet::PetSnapshotV1 deadSaved = deadRoundTrip.snapshot();
    assert(Pet::isValidPetSnapshot(deadSaved));
    assert(deadSaved.healthState == Pet::HealthState::Dead);
    assert(deadSaved.deathCause == Pet::DeathCause::UntreatedSickness);

    char generatedName[Pet::kPetNameMaxLength + 1]{};
    Pet::generateAbbName(0, generatedName);
    assert(strlen(generatedName) == 6);
    assert(generatedName[0] >= 'A' && generatedName[0] <= 'Z');
    assert(generatedName[2] == generatedName[4]);
    assert(generatedName[3] == generatedName[5]);
    assert(!(generatedName[0] + ('a' - 'A') == generatedName[2] &&
             generatedName[1] == generatedName[3]));

    Pet::PetData namedEgg;
    assert(namedEgg.startNewEgg(42, generatedName));
    assert(namedEgg.petId() == 42 && strcmp(namedEgg.name(), generatedName) == 0);
    assert(namedEgg.lifeStage() == Pet::LifeStage::Egg);
    assert(!namedEgg.startNewEgg(0, generatedName));
    assert(namedEgg.petId() == 42);

    Pet::PetData rememberedPet;
    assert(rememberedPet.startNewEgg(7, "Tamama"));
    rememberedPet.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
    rememberedPet.setDead(true);
    Storage::Memorials memorials;
    assert(memorials.append(rememberedPet));
    assert(memorials.count() == 1 && memorials.containsPet(7));
    assert(memorials.containsName("Tamama"));
    assert(memorials.append(rememberedPet) && memorials.count() == 1);
    assert(memorials.highestPetId() == 7);
    assert(memorials.at(0)->ageSeconds == rememberedPet.ageSeconds());
    assert(memorials.remove(0) && memorials.count() == 0);
    for (uint8_t i = 0; i < Storage::kMemorialCapacity; ++i) {
        Storage::MemorialRecord item{};
        item.petId = static_cast<uint64_t>(i) + 1;
        memcpy(item.name, "Tamama", 7);
        item.speciesId = Pet::SpeciesId::Bird;
        item.ageSeconds = i;
        assert(memorials.append(item));
    }
    Storage::MemorialRecord overflow = *memorials.at(0);
    overflow.petId = 100;
    assert(!memorials.append(overflow));
    assert(memorials.count() == Storage::kMemorialCapacity);

    Pet::PetClock clock;
    Pet::PetData deep;
    deep.setMood(50);
    assert(deep.beginDeepSleep());
    assert(Pet::isValidPetSnapshot(deep.snapshot()));
    deep.advanceSeconds(30);
    assert(deep.ageSeconds() == 0);
    deep.advanceSleepSeconds(Pet::PetData::kSleepMoodRecoverySeconds - 1);
    const auto deepSaved = deep.snapshot();
    const auto recoveryRemainder = deep.sleepRecoveryProgress();
    Pet::PetData deepRestored;
    assert(deepRestored.restore(deepSaved));
    assert(!deepRestored.restoreSleepRecoveryProgress(Pet::PetData::kSleepMoodRecoveryProgress));
    assert(deepRestored.restoreSleepRecoveryProgress(recoveryRemainder));
    deepRestored.advanceSleepSeconds(1);
    assert(deepRestored.mood() == 51);
    assert(deepRestored.ageSeconds() == Pet::PetData::kSleepMoodRecoverySeconds);
    assert(deepRestored.wake());
    assert(!deepRestored.restoreSleepRecoveryProgress(1));
    assert(!namedEgg.beginDeepSleep());
    Pet::PetData shortSleeps;
    shortSleeps.setMood(50);
    assert(shortSleeps.beginDeepSleep());
    shortSleeps.advanceSleepSeconds(Pet::PetData::kSleepMoodRecoverySeconds - 1);
    assert(shortSleeps.wake());
    assert(shortSleeps.beginNormalSleep());
    shortSleeps.advanceSleepSeconds(1);
    assert(shortSleeps.mood() == 51);
    Pet::PetData deepNeglect;
    deepNeglect.setSatiety(0);
    assert(deepNeglect.beginDeepSleep());
    deepNeglect.advanceSleepSeconds(Pet::PetData::kSicknessExposureSeconds +
        Pet::PetData::kDeathAfterSickAwakeSeconds + 100);
    assert(deepNeglect.isDead());
    assert(deepNeglect.ageSeconds() == Pet::PetData::kSicknessExposureSeconds +
        Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(Pet::isValidPetSnapshot(deepNeglect.snapshot()));
    assert(clock.consumeElapsedSeconds(UINT32_MAX - 500) == 0);
    assert(clock.consumeElapsedSeconds(498) == 0);
    assert(clock.consumeElapsedSeconds(499) == 1);
    assert(clock.consumeElapsedSeconds(1499) == 1);
    return 0;
}
