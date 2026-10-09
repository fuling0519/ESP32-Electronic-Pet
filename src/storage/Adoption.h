#pragma once

#include "storage/Save.h"

namespace Storage {
// Keep one candidate across save failures. RNG is consumed by the application
// only before prepare(), so retrying the same adoption cannot reroll the egg.
class Adoption {
public:
    bool prepared() const { return prepared_; }
    bool prepare(uint64_t id, const char* name, uint32_t randomBits) {
        if (prepared_) return false;
        prepared_ = candidate_.startNewEgg(id, name, Pet::speciesForNewEgg(randomBits));
        return prepared_;
    }
    bool commit(Save& save, Pet::PetData& current) {
        if (!prepared_ || !save.save(candidate_) || !current.restore(candidate_.snapshot())) return false;
        prepared_ = false;
        return true;
    }
private:
    Pet::PetData candidate_;
    bool prepared_ = false;
};
}  // namespace Storage
