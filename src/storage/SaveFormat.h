#pragma once

#include <stdint.h>

#include "pet/PetSnapshot.h"

namespace Storage {

// "PET1" in an endian-independent logical value. The serializer will write
// each field explicitly rather than dumping this C++ structure as raw bytes.
constexpr uint32_t kPetSaveMagic = 0x50455431UL;
constexpr uint16_t kPetSaveSchemaVersion = 1;
constexpr uint16_t kPetSavePayloadSize = 92;
constexpr uint16_t kPetSaveRecordSize = 108;

struct SaveEnvelopeV1 {
    uint32_t magic;
    uint16_t schemaVersion;
    uint16_t payloadSize;
    uint32_t sequence;
    Pet::PetSnapshotV1 pet;
    uint32_t crc32;
};

}  // namespace Storage
