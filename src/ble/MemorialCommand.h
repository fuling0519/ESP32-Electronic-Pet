#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>
#include "pet/PetData.h"
#include "storage/Memorials.h"

namespace Ble {

// get_memorials/delete_memorial (feature memorials). Reading needs no op and works in any
// activity; deletion uses the shared op cache and two-step reply, and the web confirms (D3).
constexpr uint8_t kMemorialPageSize = 5;

// offset of a get_memorials whose v/id/cmd were already validated: "ok", missing_field (absent or
// not an integer) or invalid_value (negative). An offset beyond 32 bits reads an empty page.
const char* parseGetMemorials(JsonVariantConst message, uint32_t& offset);
// One page in storage order: hash (as status memorial_hash), total, offset and up to 5 items.
// An offset at or past total gives an empty items array.
bool makeMemorialsPageJson(uint32_t id, const Storage::Memorials& memorials, const Pet::PetData& pet,
                           uint32_t offset, char* out, size_t cap, uint16_t& len);

// The current ended pet's own record (not adopted away yet) cannot be deleted, on the device or the web.
bool memorialProtected(const Storage::MemorialRecord& record, const Pet::PetData& pet);
// Storage index of the record with this pet id, or -1.
int findMemorial(const Storage::Memorials& memorials, uint64_t petId);

struct MemorialRequest {
    uint32_t commandId = 0;
    uint64_t memorialPetId = 0;  // The op target.
    uint64_t op = 0;
};
// memorial_pet_id and op: "ok", missing_field or invalid_value.
const char* parseDeleteMemorial(JsonVariantConst message, uint32_t id, MemorialRequest& out);
// After the op cache: not_found, not_allowed (protected), then busy (activity other than idle,
// menu, sleeping or ended; the device's delete confirmation page is busy). "ok" when accepted.
const char* deleteMemorialEligibility(uint64_t memorialPetId, const Pet::PetData& pet,
                                      const Storage::Memorials& memorials, const char* activity);

struct MemorialOutcome {
    bool done = false;
    const char* code = nullptr;  // failed only: save_failed (static text).
};
MemorialOutcome memorialDone();
MemorialOutcome memorialFailed(const char* code);
// action "delete_memorial"; done carries an empty changes object.
bool makeMemorialResultJson(uint32_t id, uint64_t op, const MemorialOutcome& outcome,
                            char* out, size_t cap, uint16_t& len);

}  // namespace Ble
