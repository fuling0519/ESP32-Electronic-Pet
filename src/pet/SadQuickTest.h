#pragma once
#if defined(PET_SAD_TEST_MODE)
#include "pet/PetData.h"

namespace Pet {
// Only the dedicated test build calls this; never changes identity or life stage.
inline bool applySadTestPreset(PetData& pet, char command) {
    if (command < '0' || command > '9' || pet.isEnded() ||
        pet.lifeStage() == LifeStage::Egg || pet.sleepMode() != SleepMode::Awake) return false;
    if (command <= '5') {
        pet.setSick(false);
        pet.setMood(80); pet.setSatiety(80); pet.setCleanliness(100);
        switch (command) {
            case '1': pet.setMood(40); break;
            case '2': pet.setSatiety(25); break;
            case '3': pet.setCleanliness(25); break;
            case '4': pet.setSick(true); break;
            case '5': pet.setMood(40); pet.setSatiety(25); pet.setCleanliness(25); break;
            default: break;
        }
    } else if (command == '6') pet.setMood(44);
    else if (command == '7') pet.setMood(45);
    else if (command == '8') { pet.setSatiety(29); pet.setCleanliness(29); }
    else { pet.setSatiety(30); pet.setCleanliness(30); }
    return true;
}
} // namespace Pet
#endif
