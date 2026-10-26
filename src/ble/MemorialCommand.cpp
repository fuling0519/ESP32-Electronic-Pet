#include "ble/MemorialCommand.h"
#include <stdio.h>
#include <string.h>
#include "ble/CareCommand.h"
#include "ble/GameCommand.h"
#include "ble/Messages.h"

namespace Ble {
namespace {
const char* stageName(uint8_t stage) {
    switch (stage) {
        case static_cast<uint8_t>(Pet::LifeStage::Egg): return "egg";
        case static_cast<uint8_t>(Pet::LifeStage::Baby): return "baby";
        case static_cast<uint8_t>(Pet::LifeStage::Adult): return "adult";
    }
    return "unknown";  // kUnknownMemorialStage: old gravestones without a stage.
}
bool activityAllows(const char* a) {
    return a && (strcmp(a, "idle") == 0 || strcmp(a, "menu") == 0 ||
                 strcmp(a, "sleeping") == 0 || strcmp(a, "ended") == 0);
}
// Pointers only: ids, ages and hash live in the builder's buffers, names in the archive.
constexpr size_t kPageDoc = JSON_OBJECT_SIZE(7) + JSON_ARRAY_SIZE(kMemorialPageSize) +
    kMemorialPageSize * JSON_OBJECT_SIZE(7) + 32;
}

const char* parseGetMemorials(JsonVariantConst message, uint32_t& offset) {
    JsonVariantConst o = message["offset"];
    if (o.is<uint32_t>()) offset = o.as<uint32_t>();
    else if (o.is<long long>() && o.as<long long>() < 0) return "invalid_value";
    else if (o.is<unsigned long long>()) offset = UINT32_MAX;  // Past any archive: an empty page.
    else return "missing_field";
    return "ok";
}

bool memorialProtected(const Storage::MemorialRecord& r, const Pet::PetData& pet) {
    return pet.isEnded() && r.petId == pet.petId();
}

int findMemorial(const Storage::Memorials& memorials, uint64_t petId) {
    for (uint8_t i = 0; i < memorials.count(); ++i) if (memorials.at(i)->petId == petId) return i;
    return -1;
}

bool makeMemorialsPageJson(uint32_t id, const Storage::Memorials& memorials, const Pet::PetData& pet,
                           uint32_t offset, char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<kPageDoc> d;
    char hash[9], ids[kMemorialPageSize][21], ages[kMemorialPageSize][21];
    snprintf(hash, sizeof(hash), "%08lx", static_cast<unsigned long>(Storage::memorialHash(memorials)));
    const uint8_t total = memorials.count();
    d["v"] = 1; d["type"] = "memorials"; d["id"] = id; d["hash"] = static_cast<const char*>(hash);
    d["total"] = total; d["offset"] = offset;
    JsonArray items = d.createNestedArray("items");
    for (uint8_t n = 0; n < kMemorialPageSize && offset < total && n < total - offset; ++n) {
        const Storage::MemorialRecord& r = *memorials.at(static_cast<uint8_t>(offset + n));
        snprintf(ids[n], sizeof(ids[n]), "%llu", static_cast<unsigned long long>(r.petId));
        snprintf(ages[n], sizeof(ages[n]), "%llu", static_cast<unsigned long long>(r.ageSeconds));
        JsonObject item = items.createNestedObject();
        item["pet_id"] = static_cast<const char*>(ids[n]); item["name"] = static_cast<const char*>(r.name);
        item["species_id"] = static_cast<uint8_t>(r.speciesId); item["age_seconds"] = static_cast<const char*>(ages[n]);
        item["kind"] = r.kind == Storage::FarewellKind::Departed ? "departed" : "resting";
        item["stage"] = stageName(r.stage); item["protected"] = memorialProtected(r, pet);
    }
    if (d.overflowed()) return false;
    const size_t written = serializeJson(d, out, cap);
    if (!written || written >= cap || written > kMaxMessageBytes) return false;
    len = static_cast<uint16_t>(written);
    return true;
}

const char* parseDeleteMemorial(JsonVariantConst message, uint32_t id, MemorialRequest& out) {
    JsonVariantConst pet = message["memorial_pet_id"], op = message["op"];
    if (!pet.is<const char*>() || !op.is<const char*>()) return "missing_field";
    MemorialRequest r; r.commandId = id;
    if (!parseOp(op.as<const char*>(), r.op) || !parseDecimal(pet.as<const char*>(), r.memorialPetId))
        return "invalid_value";
    out = r;
    return "ok";
}

const char* deleteMemorialEligibility(uint64_t memorialPetId, const Pet::PetData& pet,
                                      const Storage::Memorials& memorials, const char* activity) {
    const int index = findMemorial(memorials, memorialPetId);
    if (index < 0) return "not_found";
    if (memorialProtected(*memorials.at(static_cast<uint8_t>(index)), pet)) return "not_allowed";
    if (!activityAllows(activity)) return "busy";
    return "ok";
}

MemorialOutcome memorialDone() { MemorialOutcome o; o.done = true; return o; }
MemorialOutcome memorialFailed(const char* code) { MemorialOutcome o; o.code = code; return o; }

bool makeMemorialResultJson(uint32_t id, uint64_t op, const MemorialOutcome& o,
                            char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(0) + 32> d;
    char opText[17];
    snprintf(opText, sizeof(opText), "%016llx", static_cast<unsigned long long>(op));
    d["v"] = 1; d["type"] = "action_result"; d["id"] = id; d["op"] = static_cast<const char*>(opText);
    d["action"] = "delete_memorial";
    if (o.done) { d["outcome"] = "done"; d.createNestedObject("changes"); }
    else { d["outcome"] = "failed"; d["code"] = o.code ? o.code : "save_failed"; }
    if (d.overflowed()) return false;
    const size_t written = serializeJson(d, out, cap);
    if (!written || written >= cap || written > kMaxMessageBytes) return false;
    len = static_cast<uint16_t>(written);
    return true;
}

}  // namespace Ble
