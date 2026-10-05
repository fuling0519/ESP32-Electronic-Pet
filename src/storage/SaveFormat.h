#pragma once

#include <stdint.h>
#include <stddef.h>

#include "pet/PetSnapshot.h"
#include "storage/Memorials.h"

namespace Storage {

// "PET1" in an endian-independent logical value. The serializer will write
// each field explicitly rather than dumping this C++ structure as raw bytes.
constexpr uint32_t kPetSaveMagic = 0x50455431UL;
constexpr uint16_t kPetSaveSchemaVersion = 2;
constexpr uint16_t kPetSavePayloadSize = 93;
constexpr uint16_t kPetSaveRecordSize = 109;
constexpr size_t kMemorialSaveRecordSize = 12 + 1 + kMemorialCapacity *
    (8 + (Pet::kPetNameMaxLength + 1) + 1 + 8 + 2) + 4;

struct SaveEnvelopeV1 {
    uint32_t magic;
    uint16_t schemaVersion;
    uint16_t payloadSize;
    uint32_t sequence;
    Pet::PetSnapshotV1 pet;
    uint32_t crc32;
};

}  // namespace Storage
