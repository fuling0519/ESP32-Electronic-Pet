#pragma once

#include "storage/Save.h"

namespace Storage {

// Used only by the one-off sample firmware and native tests.
inline bool ensureMemorialSample(const Pet::PetData& current, Memorials& memorials, Save& save) {
    if (memorials.containsName("TestRIP")) return true;
    // Keep the death overflow slot reserved for the real current pet.
    if (!save.canWriteMemorials() || memorials.count() >= kMemorialLimit) return false;
    uint64_t highest = memorials.highestPetId();
    if (current.petId() > highest) highest = current.petId();
    if (highest == UINT64_MAX) return false;
    Pet::PetData sample;
    if (!sample.startNewEgg(highest + 1, "TestRIP")) return false;
    auto snapshot = sample.snapshot();
    snapshot.lifeStage = Pet::LifeStage::Adult;
    snapshot.ageSeconds = 3ULL * 86400 + 2 * 3600;
    if (!sample.restore(snapshot)) return false;
    sample.setDead(true);
    return save.appendMemorial(sample, memorials);
}

}  // namespace Storage
