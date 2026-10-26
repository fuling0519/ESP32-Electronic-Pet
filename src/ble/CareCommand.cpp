#include "ble/CareCommand.h"
#include <stdio.h>
#include <string.h>
#include "ble/GameCommand.h"
#include "ble/Messages.h"

namespace Ble {
namespace {
const char* const kActions[] = {"feed", "clean", "treat", "sleep", "wake"};
uint8_t gain(uint8_t before, uint8_t after) { return after > before ? after - before : 0; }
}

const char* careActionName(CareAction a) {
    const uint8_t i = static_cast<uint8_t>(a);
    return i < 5 ? kActions[i] : "unknown";
}

bool parseOp(const char* text, uint64_t& op) {
    if (!text || strlen(text) != 16) return false;
    uint64_t value = 0;
    for (const char* p = text; *p; ++p) {
        const char c = *p;
        if (c >= '0' && c <= '9') value = value << 4 | static_cast<uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') value = value << 4 | static_cast<uint64_t>(c - 'a' + 10);
        else return false;
    }
    op = value;
    return true;
}

const char* parseCareCommand(JsonVariantConst message, uint32_t id, CareRequest& out) {
    JsonVariantConst action = message["action"], pet = message["pet_id"], op = message["op"];
    if (!action.is<const char*>() || !pet.is<const char*>() || !op.is<const char*>()) return "missing_field";
    CareRequest request;
    request.commandId = id;
    if (!parseOp(op.as<const char*>(), request.op)) return "invalid_value";
    uint8_t i = 0;
    while (i < 5 && strcmp(action.as<const char*>(), kActions[i]) != 0) ++i;
    if (i == 5) return "invalid_value";
    request.action = static_cast<CareAction>(i);
    if (!parseDecimal(pet.as<const char*>(), request.petId)) return "invalid_value";
    out = request;
    return "ok";
}

bool carePetAllowed(CareAction a, const Pet::PetData& pet) {
    if (pet.isEnded()) return false;
    if (a == CareAction::Wake) return pet.sleepMode() == Pet::SleepMode::Normal;
    if (pet.lifeStage() == Pet::LifeStage::Egg || pet.sleepMode() != Pet::SleepMode::Awake) return false;
    return a != CareAction::Treat || pet.isSick();
}

const char* careEligibility(CareAction a, uint64_t petId, const Pet::PetData& pet, const char* activity) {
    if (petId != pet.petId()) return "pet_changed";
    if (!carePetAllowed(a, pet)) return "not_allowed";
    if (!activity) return "busy";
    const bool allowed = a == CareAction::Wake ? strcmp(activity, "sleeping") == 0 :
        strcmp(activity, "idle") == 0 || strcmp(activity, "menu") == 0;
    return allowed ? "ok" : "busy";
}

uint32_t totalExp(const Pet::PetData& pet) {
    uint32_t total = pet.exp();
    for (uint8_t lv = 1; lv < pet.level(); ++lv) total += Pet::PetData::expToLevelUp(lv);
    return total;
}

CareOutcome careDone(const Pet::PetData& before, const Pet::PetData& after) {
    CareOutcome o;
    o.done = true;
    o.changes.satiety = gain(before.satiety(), after.satiety());
    o.changes.cleanliness = gain(before.cleanliness(), after.cleanliness());
    o.changes.levelsGained = gain(before.level(), after.level());
    const uint32_t a = totalExp(after), b = totalExp(before);
    o.changes.exp = static_cast<uint16_t>(a > b ? a - b : 0);
    o.changes.grew = before.lifeStage() == Pet::LifeStage::Baby && after.lifeStage() == Pet::LifeStage::Adult;
    return o;
}

CareOutcome careFailed(const char* code) {
    CareOutcome o;
    o.code = code;
    return o;
}

const char* careFailureCode(const CareRequest& request, const Pet::PetData& pet) {
    return request.petId != pet.petId() ? "pet_changed" : "not_allowed";
}

bool makeActionResultJson(uint32_t id, uint64_t op, CareAction action, const CareOutcome& o,
                          char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<JSON_OBJECT_SIZE(8) + JSON_OBJECT_SIZE(3) + 32> d;
    char opText[17];
    snprintf(opText, sizeof(opText), "%016llx", static_cast<unsigned long long>(op));
    d["v"] = 1; d["type"] = "action_result"; d["id"] = id; d["op"] = opText;
    d["action"] = careActionName(action);
    if (!o.done) {
        d["outcome"] = "failed"; d["code"] = o.code ? o.code : "not_allowed";
    } else {
        d["outcome"] = "done";
        JsonObject c = d.createNestedObject("changes");
        const CareChanges& ch = o.changes;
        switch (action) {
            case CareAction::Feed: c["satiety"] = ch.satiety; c["exp"] = ch.exp; c["levels_gained"] = ch.levelsGained; break;
            case CareAction::Clean: c["cleanliness"] = ch.cleanliness; c["exp"] = ch.exp; c["levels_gained"] = ch.levelsGained; break;
            case CareAction::Treat: c["exp"] = ch.exp; c["levels_gained"] = ch.levelsGained; c["grew"] = ch.grew; break;
            case CareAction::Sleep: break;
            case CareAction::Wake: c["grew"] = ch.grew; break;
        }
    }
    if (d.overflowed()) return false;
    const size_t written = serializeJson(d, out, cap);
    if (!written || written >= cap || written > kMaxMessageBytes) return false;
    len = static_cast<uint16_t>(written);
    return true;
}

bool PendingTreatment::interrupted(bool uiTreating, const Pet::PetData& pet, CareOutcome& out) {
    if (!active_ || uiTreating) return false;
    active_ = false;
    out = careFailed(careFailureCode(request_, pet));
    return true;
}

}  // namespace Ble
