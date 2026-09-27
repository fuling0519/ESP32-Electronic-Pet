#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "pet/PetClock.h"
#include "pet/PetData.h"

int main() {
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

    Pet::PetClock clock;
    assert(clock.consumeElapsedSeconds(UINT32_MAX - 500) == 0);
    assert(clock.consumeElapsedSeconds(498) == 0);
    assert(clock.consumeElapsedSeconds(499) == 1);
    assert(clock.consumeElapsedSeconds(1499) == 1);
    return 0;
}
