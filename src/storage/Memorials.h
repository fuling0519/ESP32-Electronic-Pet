#pragma once

#include <stdint.h>

#include "pet/PetSnapshot.h"

namespace Pet { class PetData; }

namespace Storage {

constexpr uint8_t kMemorialLimit = 32;
// One overflow slot guarantees that the current death can be recorded before
// the player explicitly deletes an older memorial to adopt again.
constexpr uint8_t kMemorialCapacity = kMemorialLimit + 1;

struct MemorialRecord {
    uint64_t petId;
    char name[Pet::kPetNameMaxLength + 1];
    Pet::SpeciesId speciesId;
    uint64_t ageSeconds;
};

bool isValidMemorial(const MemorialRecord& memorial);

class Memorials {
public:
    uint8_t count() const;
    const MemorialRecord* at(uint8_t index) const;
    bool containsPet(uint64_t petId) const;
    bool containsName(const char* name) const;
    uint64_t highestPetId() const;
    bool append(const MemorialRecord& memorial);
    bool append(const Pet::PetData& pet);
    bool remove(uint8_t index);
    void clear();

private:
    MemorialRecord records_[kMemorialCapacity]{};
    uint8_t count_ = 0;
};

}  // namespace Storage
