#pragma once
#include <stddef.h>
#include <stdint.h>
#include "pet/PetData.h"

namespace Pet { class SadState; }
namespace Storage { class Memorials; }

namespace Ble {

// Reassembled JSON limit shared with the transport (PROTOCOL.md max_message_bytes).
constexpr uint16_t kMaxMessageBytes = 1024;

// Pet sync additions to the status snapshot; a null source leaves its fields out.
struct StatusSources {
    const Pet::SadState* sad = nullptr;              // pet.sad_reasons
    const Storage::Memorials* memorials = nullptr;   // memorial_count + memorial_hash
    const char* sound = nullptr;                     // "on" or "muted"; the saved mode (feature sound)
};

// JSON builders without NimBLE, so native tests can check contents and size.
// Each returns false on overflow or when the message would exceed kMaxMessageBytes.
bool makeStatusJson(const Pet::PetData& pet, const char* deviceId, const char* activity,
                    const StatusSources& sources, char* out, size_t cap, uint16_t& len);
bool makeDeviceInfoJson(uint32_t id, const char* name, const char* deviceId,
                        char* out, size_t cap, uint16_t& len);

}  // namespace Ble
