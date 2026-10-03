#include <assert.h>
#include <limits.h>
#include <initializer_list>
#include "pet/PetData.h"

using Pet::PetData;

static void sameState(const PetData& a, const PetData& b) {
    const auto x = a.snapshot(), y = b.snapshot();
    assert(x.lifeStage == y.lifeStage && x.healthState == y.healthState);
    assert(x.ageSeconds == y.ageSeconds && x.sleepMode == y.sleepMode);
    assert(x.satiety == y.satiety && x.cleanliness == y.cleanliness && x.mood == y.mood);
    assert(x.satietyRemainderSeconds == y.satietyRemainderSeconds);
    assert(x.cleanlinessRemainderSeconds == y.cleanlinessRemainderSeconds);
    assert(x.moodRemainderSeconds == y.moodRemainderSeconds);
    assert(x.dangerSeconds == y.dangerSeconds && x.sickAwakeSeconds == y.sickAwakeSeconds);
    assert(a.sleepRecoveryProgress() == b.sleepRecoveryProgress());
}

int main() {
    // 50 is low, 51 is normal, for both needs and both life stages.
    for (bool baby : {false, true}) {
        for (int satiety : {50, 51}) for (int cleanliness : {50, 51}) {
            PetData p;
            if (baby) { p.startNewEgg(); p.advanceSeconds(PetData::kEggHatchAgeSeconds); }
            auto s = p.snapshot();
            s.mood = 80; s.satiety = satiety; s.cleanliness = cleanliness;
            s.moodRemainderSeconds = (baby ? PetData::kBabyMoodDecaySeconds : PetData::kMoodDecaySeconds) - 1;
            assert(p.restore(s));
            p.advanceSeconds(1);
            assert(p.mood() == 79 - (satiety == 50) - (cleanliness == 50));
            // Feeding and cleaning immediately restore the normal tick rate.
            assert(p.feed() && p.clean());
            s = p.snapshot(); s.moodRemainderSeconds = (baby ? PetData::kBabyMoodDecaySeconds : PetData::kMoodDecaySeconds) - 1;
            assert(p.restore(s));
            const auto before = p.mood(); p.advanceSeconds(1);
            assert(p.mood() == before - 1);
        }
    }

    // Coincident events: decay the need to 50 before the awake mood tick.
    PetData boundary;
    auto s = boundary.snapshot(); s.satiety = 51;
    s.satietyRemainderSeconds = PetData::kSatietyDecaySeconds - 1;
    s.moodRemainderSeconds = PetData::kMoodDecaySeconds - 1;
    assert(boundary.restore(s)); boundary.advanceSeconds(1);
    assert(boundary.satiety() == 50 && boundary.mood() == 78);

    for (int low = 0; low <= 2; ++low) {
        PetData p; p.setMood(80);
        if (low) p.setSatiety(50);
        if (low == 2) p.setCleanliness(50);
        assert(p.beginNormalSleep());
        const auto interval = PetData::kSleepMoodRecoverySeconds * (low == 1 ? 2 : 1);
        p.advanceSleepSeconds(interval - 1); assert(p.mood() == 80);
        p.advanceSleepSeconds(1); assert(p.mood() == (low == 2 ? 80 : 81));
    }

    // Preserve fractional progress across rate changes, paused sessions and RTC.
    PetData progress; progress.setMood(50); assert(progress.beginNormalSleep());
    progress.advanceSleepSeconds(PetData::kSleepMoodRecoverySeconds / 2);
    const auto earned = progress.sleepRecoveryProgress();
    assert(progress.wake()); progress.setSatiety(50); progress.setCleanliness(50);
    assert(progress.beginDeepSleep()); progress.advanceSleepSeconds(100);
    assert(progress.sleepRecoveryProgress() == earned && progress.mood() == 50);
    PetData resumed; assert(resumed.restore(progress.snapshot()));
    assert(resumed.restoreSleepRecoveryProgress(earned));
    assert(!resumed.restoreSleepRecoveryProgress(PetData::kSleepMoodRecoveryProgress));
    assert(resumed.wake()); assert(resumed.clean()); assert(resumed.beginNormalSleep());
    resumed.advanceSleepSeconds(PetData::kSleepMoodRecoverySeconds);
    assert(resumed.mood() == 51);

    PetData odd; odd.setSatiety(50); odd.setMood(50); assert(odd.beginDeepSleep());
    odd.advanceSleepSeconds(1); assert(odd.sleepRecoveryProgress() == 1);
    PetData oddRestored; assert(oddRestored.restore(odd.snapshot()));
    assert(oddRestored.restoreSleepRecoveryProgress(1));
    odd.advanceSleepSeconds(17); oddRestored.advanceSleepSeconds(17);
    sameState(odd, oddRestored);

    // A normal first 10 minutes plus 20 minutes at half speed earns one point.
    // This golden case uses production timing rather than the accelerated modes.
#if !defined(PET_SLEEP_TEST_MODE) && !defined(PET_DEEP_SLEEP_TEST_MODE)
    PetData crossing; crossing.setSatiety(51); crossing.setMood(50);
    assert(crossing.beginDeepSleep()); crossing.advanceSleepSeconds(600);
    assert(crossing.satiety() == 50 && crossing.sleepRecoveryProgress() == 1200);
    crossing.advanceSleepSeconds(1199); assert(crossing.mood() == 50);
    crossing.advanceSleepSeconds(1); assert(crossing.mood() == 51);
#endif

    // Batch settlement must match live one-second updates through thresholds,
    // growth, sickness and death, including the 48-hour deep-sleep cap.
    for (bool baby : {false, true}) for (bool sleeping : {false, true}) {
        for (int need : {0, 50, 51, 52, 100}) {
            PetData batch;
            if (baby) {
                batch.startNewEgg(); batch.advanceSeconds(PetData::kEggHatchAgeSeconds);
                auto saved = batch.snapshot(); saved.ageSeconds = PetData::kAdultAgeSeconds - 301;
                assert(batch.restore(saved));
            }
            batch.setSatiety(need); batch.setCleanliness(need == 52 ? 51 : need);
            batch.setMood(99);
            auto partial = batch.snapshot();
            partial.satietyRemainderSeconds = (baby ? PetData::kBabySatietyDecaySeconds : PetData::kSatietyDecaySeconds) - 1;
            partial.cleanlinessRemainderSeconds = (baby ? PetData::kBabyCleanlinessDecaySeconds : PetData::kCleanlinessDecaySeconds) - 2;
            partial.moodRemainderSeconds = (baby ? PetData::kBabyMoodDecaySeconds : PetData::kMoodDecaySeconds) - 3;
            assert(batch.restore(partial));
            if (sleeping) assert(batch.beginDeepSleep());
            PetData live; assert(live.restore(batch.snapshot()));
            const uint32_t duration = 48UL * 60 * 60;
            if (sleeping) batch.advanceSleepSeconds(duration); else batch.advanceSeconds(duration);
            for (uint32_t i = 0; i < duration; ++i) {
                if (sleeping) live.advanceSleepSeconds(1); else live.advanceSeconds(1);
            }
            sameState(batch, live);
            assert(Pet::isValidPetSnapshot(batch.snapshot()));
        }
    }
    PetData floor; floor.setMood(1); floor.setSatiety(0); floor.setCleanliness(0);
    floor.advanceSeconds(PetData::kMoodDecaySeconds); assert(floor.mood() == 0);
    PetData ceiling; ceiling.setMood(100); assert(ceiling.beginNormalSleep());
    ceiling.advanceSleepSeconds(PetData::kSleepMoodRecoverySeconds); assert(ceiling.mood() == 100);
    PetData dead; dead.setSatiety(0); assert(dead.beginDeepSleep());
    dead.advanceSleepSeconds(UINT32_MAX); assert(dead.isDead());
    assert(dead.ageSeconds() == PetData::kSicknessExposureSeconds + PetData::kDeathAfterSickAwakeSeconds);
    return 0;
}
