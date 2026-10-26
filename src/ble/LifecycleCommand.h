#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>
#include "pet/PetData.h"

namespace Ble {

// depart/adopt (feature lifecycle): same op and two-step reply as care. The web confirms (D3);
// the firmware runs the device path directly, without a confirmation screen.
enum class LifecycleAction : uint8_t { Depart, Adopt };
struct LifecycleRequest {
    uint32_t commandId = 0;
    LifecycleAction action = LifecycleAction::Depart;
    uint64_t petId = 0;  // Depart: the living pet. Adopt: the ended current pet (the op target).
    uint64_t op = 0;
};
const char* lifecycleActionName(LifecycleAction action);
// cmd "depart" or "adopt".
bool isLifecycleCommand(const char* cmd, LifecycleAction& action);
// pet_id and op of a command whose v/id/cmd were already validated: "ok", missing_field or invalid_value.
const char* parseLifecycleCommand(JsonVariantConst message, uint32_t id, LifecycleAction action,
                                  LifecycleRequest& out);

// After the op cache: pet_changed, not_allowed, memorial_full, then busy; "ok" when accepted.
// depart: a living awake baby or adult (sick allowed), fewer than kMemorialLimit memorials,
// activity idle or menu.
const char* departEligibility(uint64_t petId, const Pet::PetData& pet, const char* activity,
                              uint8_t memorialCount);
// adopt: the current pet dead or departed, at most kMemorialLimit memorials (the overflow slot
// in use is memorial_full), its memorial saved and activity ended (an unsaved memorial is busy:
// the device is still retrying it).
const char* adoptEligibility(uint64_t petId, const Pet::PetData& pet, const char* activity,
                             uint8_t memorialCount, bool memorialReady);

struct LifecycleOutcome {
    bool done = false;
    const char* code = nullptr;  // failed only: save_failed (static text).
    bool memorialSaved = false;  // depart
    uint64_t newPetId = 0;       // adopt
    char name[Pet::kPetNameMaxLength + 1]{};
    uint8_t speciesId = 0;
};
LifecycleOutcome departDone(bool memorialSaved);
LifecycleOutcome adoptDone(const Pet::PetData& newPet);
LifecycleOutcome lifecycleFailed(const char* code);

// action is "depart" or "adopt"; changes {"memorial_saved"} or {"new_pet_id","name","species_id"}.
bool makeLifecycleResultJson(uint32_t id, uint64_t op, LifecycleAction action, const LifecycleOutcome& outcome,
                             char* out, size_t cap, uint16_t& len);

}  // namespace Ble
