#include "ble/LifecycleCommand.h"
#include <stdio.h>
#include <string.h>
#include "ble/CareCommand.h"
#include "ble/GameCommand.h"
#include "ble/Messages.h"
#include "storage/Memorials.h"

namespace Ble {

const char* lifecycleActionName(LifecycleAction a) { return a == LifecycleAction::Adopt ? "adopt" : "depart"; }

bool isLifecycleCommand(const char* cmd, LifecycleAction& action) {
    if (!cmd) return false;
    if (strcmp(cmd, "depart") == 0) action = LifecycleAction::Depart;
    else if (strcmp(cmd, "adopt") == 0) action = LifecycleAction::Adopt;
    else return false;
    return true;
}

const char* parseLifecycleCommand(JsonVariantConst message, uint32_t id, LifecycleAction action,
                                  LifecycleRequest& out) {
    JsonVariantConst pet = message["pet_id"], op = message["op"];
    if (!pet.is<const char*>() || !op.is<const char*>()) return "missing_field";
    LifecycleRequest request;
    request.commandId = id; request.action = action;
    if (!parseOp(op.as<const char*>(), request.op)) return "invalid_value";
    if (!parseDecimal(pet.as<const char*>(), request.petId)) return "invalid_value";
    out = request;
    return "ok";
}

const char* departEligibility(uint64_t petId, const Pet::PetData& pet, const char* activity, uint8_t memorialCount) {
    if (petId != pet.petId()) return "pet_changed";
    if (pet.isEnded() || pet.lifeStage() == Pet::LifeStage::Egg ||
        pet.sleepMode() != Pet::SleepMode::Awake) return "not_allowed";
    if (memorialCount >= Storage::kMemorialLimit) return "memorial_full";
    if (!activity || (strcmp(activity, "idle") != 0 && strcmp(activity, "menu") != 0)) return "busy";
    return "ok";
}

const char* adoptEligibility(uint64_t petId, const Pet::PetData& pet, const char* activity,
                             uint8_t memorialCount, bool memorialReady) {
    if (petId != pet.petId()) return "pet_changed";
    if (!pet.isEnded()) return "not_allowed";
    if (memorialCount > Storage::kMemorialLimit) return "memorial_full";
    if (!memorialReady || !activity || strcmp(activity, "ended") != 0) return "busy";
    return "ok";
}

LifecycleOutcome departDone(bool memorialSaved) {
    LifecycleOutcome o; o.done = true; o.memorialSaved = memorialSaved;
    return o;
}

LifecycleOutcome adoptDone(const Pet::PetData& pet) {
    LifecycleOutcome o; o.done = true; o.newPetId = pet.petId();
    strncpy(o.name, pet.name(), Pet::kPetNameMaxLength); o.name[Pet::kPetNameMaxLength] = '\0';
    o.speciesId = static_cast<uint8_t>(pet.speciesId());
    return o;
}

LifecycleOutcome lifecycleFailed(const char* code) {
    LifecycleOutcome o; o.code = code;
    return o;
}

bool makeLifecycleResultJson(uint32_t id, uint64_t op, LifecycleAction action, const LifecycleOutcome& o,
                             char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<JSON_OBJECT_SIZE(8) + JSON_OBJECT_SIZE(3) + 96> d;
    char opText[17], petText[21];
    snprintf(opText, sizeof(opText), "%016llx", static_cast<unsigned long long>(op));
    d["v"] = 1; d["type"] = "action_result"; d["id"] = id; d["op"] = opText;
    d["action"] = lifecycleActionName(action);
    if (!o.done) {
        d["outcome"] = "failed"; d["code"] = o.code ? o.code : "save_failed";
    } else {
        d["outcome"] = "done";
        JsonObject c = d.createNestedObject("changes");
        if (action == LifecycleAction::Depart) c["memorial_saved"] = o.memorialSaved;
        else {
            snprintf(petText, sizeof(petText), "%llu", static_cast<unsigned long long>(o.newPetId));
            c["new_pet_id"] = petText; c["name"] = static_cast<const char*>(o.name); c["species_id"] = o.speciesId;
        }
    }
    if (d.overflowed()) return false;
    const size_t written = serializeJson(d, out, cap);
    if (!written || written >= cap || written > kMaxMessageBytes) return false;
    len = static_cast<uint16_t>(written);
    return true;
}

}  // namespace Ble
