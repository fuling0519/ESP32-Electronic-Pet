#include "ble/Messages.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include "pet/SadState.h"
#include "storage/Memorials.h"

#ifndef PET_FIRMWARE_VERSION
#define PET_FIRMWARE_VERSION "development"
#endif

namespace Ble {
namespace {
const char* stage(Pet::LifeStage v) {
    switch (v) { case Pet::LifeStage::Egg:return "egg"; case Pet::LifeStage::Baby:return "baby"; case Pet::LifeStage::Adult:return "adult"; }
    return "unknown";
}
const char* health(Pet::HealthState v) {
    switch (v) { case Pet::HealthState::Healthy:return "healthy"; case Pet::HealthState::Sick:return "sick"; case Pet::HealthState::Dead:return "dead"; }
    return "unknown";
}
const char* sleep(Pet::SleepMode v) {
    switch (v) { case Pet::SleepMode::Awake:return "awake"; case Pet::SleepMode::Normal:return "normal"; case Pet::SleepMode::Deep:return "unknown"; }
    return "unknown";
}
template <typename Doc>
bool finish(Doc& d, char* out, size_t cap, uint16_t& len) {
    if (d.overflowed()) return false;
    size_t written=serializeJson(d,out,cap);
    if (!written || written>=cap || written>kMaxMessageBytes) return false;
    len=static_cast<uint16_t>(written); return true;
}
// Slots for 7 top-level and 19 pet members, the reason array, and copied id/age/hash text.
constexpr size_t kStatusDoc = JSON_OBJECT_SIZE(8) + JSON_OBJECT_SIZE(20) + JSON_ARRAY_SIZE(4) + 96;
constexpr size_t kInfoDoc = JSON_OBJECT_SIZE(12) + 2 * JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 32;
}

bool makeStatusJson(const Pet::PetData& pet, const char* deviceId, const char* activity,
                    const StatusSources& sources, char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<kStatusDoc> d;
    d["v"]=1; d["type"]="status"; d["device_id"]=deviceId;
    JsonObject p=d.createNestedObject("pet");
    char id[21], age[21];
    snprintf(id,sizeof(id),"%llu",static_cast<unsigned long long>(pet.petId()));
    snprintf(age,sizeof(age),"%llu",static_cast<unsigned long long>(pet.ageSeconds()));
    p["id"]=id; p["name"]=pet.name(); p["life_stage"]=stage(pet.lifeStage());
    p["species_id"]=static_cast<uint8_t>(pet.speciesId());
    p["satiety"]=pet.satiety(); p["mood"]=pet.mood(); p["cleanliness"]=pet.cleanliness();
    p["age_seconds"]=age; p["health"]=health(pet.healthState());
    p["is_departed"]=pet.isDeparted(); p["is_dead"]=pet.isDead(); p["sleep"]=sleep(pet.sleepMode());
    p["level"]=pet.level(); p["exp"]=pet.exp();
    p["exp_to_next_level"]=pet.expToNextLevel();
    if (const Pet::SadState* sad=sources.sad) {
        // Buffered reasons belong to one pet; a stale observation reports none. The UI refreshes
        // them before actions run, so after a farewell, adoption or treatment in this loop the
        // pet's own state still wins: eggs and ended pets report none, and sick needs isSick().
        JsonArray reasons=p.createNestedArray("sad_reasons");
        if (sad->petId()==pet.petId() && !pet.isEnded() && pet.lifeStage()!=Pet::LifeStage::Egg) {
            if (sad->mood()) reasons.add("mood");
            if (sad->satiety()) reasons.add("satiety");
            if (sad->cleanliness()) reasons.add("cleanliness");
            if (sad->sick() && pet.isSick()) reasons.add("sick");
        }
    }
    p["danger_seconds"]=pet.dangerSeconds();
    p["sick_seconds"]=pet.healthState()==Pet::HealthState::Healthy ? 0U : pet.sickAwakeSeconds();
    if (pet.isDead() && pet.deathCause()==Pet::DeathCause::UntreatedSickness) p["death_cause"]="untreated_sickness";
    d["activity"]=activity;
    if (const Storage::Memorials* m=sources.memorials) {
        char hash[9];
        snprintf(hash,sizeof(hash),"%08lx",static_cast<unsigned long>(Storage::memorialHash(*m)));
        d["memorial_count"]=m->count(); d["memorial_hash"]=hash;
    }
    return finish(d,out,cap,len);
}

bool makeDeviceInfoJson(uint32_t id, const char* name, const char* deviceId,
                        char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<kInfoDoc> info;
    info["v"]=1; info["type"]="device_info"; info["id"]=id; info["name"]=name;
    info["protocol_version"]=1; info["firmware_version"]=PET_FIRMWARE_VERSION;
    info["max_message_bytes"]=kMaxMessageBytes; info["device_id"]=deviceId;
    JsonArray features=info.createNestedArray("features"); features.add("web_game"); features.add("pet_events");
    JsonArray games=info.createNestedArray("games"); games.add("toy"); games.add("chorus");
    // Quick test builds shorten these, so the browser must not hard-code them.
    JsonObject rules=info.createNestedObject("rules");
    rules["sick_after_seconds"]=static_cast<uint32_t>(Pet::PetData::kSicknessExposureSeconds);
    rules["death_after_sick_seconds"]=static_cast<uint32_t>(Pet::PetData::kDeathAfterSickAwakeSeconds);
    rules["memorial_limit"]=static_cast<uint8_t>(Storage::kMemorialLimit);
    return finish(info,out,cap,len);
}

}  // namespace Ble
