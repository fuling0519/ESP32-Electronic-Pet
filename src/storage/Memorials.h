#pragma once

#include <stdint.h>

#include "pet/PetSnapshot.h"

namespace Pet { class PetData; }

namespace Storage {

constexpr uint8_t kMemorialLimit = 32;
// One overflow slot guarantees that the current death can be recorded before
// the player explicitly deletes an older memorial to adopt again.
constexpr uint8_t kMemorialCapacity = kMemorialLimit + 1;

enum class FarewellKind : uint8_t { Resting = 0, Departed = 1 };
constexpr uint8_t kUnknownMemorialStage = 255;

struct MemorialRecord {
    uint64_t petId;
    char name[Pet::kPetNameMaxLength + 1];
    Pet::SpeciesId speciesId;
    uint64_t ageSeconds;
    FarewellKind kind = FarewellKind::Resting;
    uint8_t stage = kUnknownMemorialStage;
};

bool isValidMemorial(const MemorialRecord& memorial);

class Memorials;
// CRC-32 (IEEE, reflected 0xEDB88320, init and final xor 0xFFFFFFFF) of the valid records in
// storage order, each encoded as 27 bytes independent of struct layout: petId u64 LE,
// name 8 bytes zero-padded, speciesId u8, ageSeconds u64 LE, kind u8, stage u8.
// Same archive content gives the same value across reboots; an empty archive is 0.
uint32_t memorialHash(const Memorials& memorials);

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
