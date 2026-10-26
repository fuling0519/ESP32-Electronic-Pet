#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>
#include "pet/PetData.h"

namespace Ble {

// care (feature care): one parsed command; the application checks eligibility and runs it.
enum class CareAction : uint8_t { Feed, Clean, Treat, Sleep, Wake };
struct CareRequest {
    uint32_t commandId = 0;
    CareAction action = CareAction::Feed;
    uint64_t petId = 0;
    uint64_t op = 0;  // 16 lowercase hex digits on the wire.
};
const char* careActionName(CareAction action);

// op: exactly 16 lowercase hex digits.
bool parseOp(const char* text, uint64_t& op);
// Fields of a care command whose v/id/cmd were already validated. Returns "ok", missing_field
// (absent or not a string) or invalid_value (op format, unknown action, pet id format).
const char* parseCareCommand(JsonVariantConst message, uint32_t id, CareRequest& out);

// PetData rules: feed/clean/sleep need a living awake baby or adult; treat also needs sickness;
// wake needs normal sleep and a living pet.
bool carePetAllowed(CareAction action, const Pet::PetData& pet);
// After the op cache: pet_changed, then not_allowed, then busy. activity is the status value
// (feed/clean/treat/sleep accept idle or menu, wake accepts sleeping). Returns "ok" when accepted.
const char* careEligibility(CareAction action, uint64_t petId, const Pet::PetData& pet, const char* activity);

// Actual increments; only the fields of the action are sent.
struct CareChanges {
    uint8_t satiety = 0, cleanliness = 0, levelsGained = 0;
    uint16_t exp = 0;
    bool grew = false;
};
struct CareOutcome {
    bool done = false;
    const char* code = nullptr;  // failed only: not_allowed, pet_changed or save_failed (static text).
    CareChanges changes{};
};
uint32_t totalExp(const Pet::PetData& pet);
CareOutcome careDone(const Pet::PetData& before, const Pet::PetData& after);
CareOutcome careFailed(const char* code);
// Why a settled-later action failed: the pet changed, or its state no longer allows it.
const char* careFailureCode(const CareRequest& request, const Pet::PetData& pet);

bool makeActionResultJson(uint32_t id, uint64_t op, CareAction action, const CareOutcome& outcome,
                          char* out, size_t cap, uint16_t& len);

// The one web treatment between acceptance and the Treat action after the pour (~1.2 s).
class PendingTreatment {
public:
    void start(const CareRequest& request) { request_ = request; active_ = true; }
    bool active() const { return active_; }
    const CareRequest& request() const { return request_; }
    void clear() { active_ = false; }
    // The pour stopped without settling (death, sleep, another screen): failed for the pet now.
    bool interrupted(bool uiTreating, const Pet::PetData& pet, CareOutcome& out);
private:
    CareRequest request_{};
    bool active_ = false;
};

}  // namespace Ble
