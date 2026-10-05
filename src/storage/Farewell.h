#pragma once

#include "storage/Save.h"

namespace Storage {
// Both writes are restartable: the terminal pet checkpoint is the durable intent.
// A reboot retries the archive append by petId before adoption is enabled.
inline bool finishFarewell(Pet::PetData& pet, Memorials& memorials, Save& save) {
    if (!pet.isEnded() || !save.save(pet)) return false;
    return save.appendMemorial(pet, memorials);
}

inline bool beginFarewell(Pet::PetData& pet, Memorials& memorials, Save& save) {
    if (!save.canWriteMemorials() || memorials.count() >= kMemorialLimit ||
        memorials.containsPet(pet.petId())) return false;
    Pet::PetData candidate = pet;
    // Establish an archive rollback slot even on a device with no deaths yet.
    // No terminal intent is committed until this preflight succeeds.
    if (!candidate.depart() || !save.saveMemorials(memorials) || !save.save(candidate)) return false;
    pet = candidate;
    return finishFarewell(pet, memorials, save);
}
}  // namespace Storage
