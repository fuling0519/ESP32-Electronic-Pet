// Pet sync phase 3 without NimBLE: depart/adopt parsing, eligibility order (memorial_full included),
// lifecycle ops in the shared op cache (replay after adoption, conflicts with care) and action_result JSON.
#include <ArduinoJson.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble/CareCommand.h"
#include "ble/LifecycleCommand.h"
#include "ble/OpCache.h"
#include "pet/PetData.h"
#include "storage/Memorials.h"

using Ble::LifecycleAction;
using Match = Ble::OpCache::Match;
constexpr uint8_t kLimit = Storage::kMemorialLimit;

static Pet::PetData living(uint64_t id, Pet::LifeStage stage) {
    Pet::PetData pet; assert(pet.startNewEgg(id, "Tamama", Pet::SpeciesId::Bird));
    auto s = pet.snapshot(); s.lifeStage = stage;
    s.ageSeconds = stage == Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds :
        stage == Pet::LifeStage::Adult ? Pet::PetData::kAdultAgeSeconds : 0;
    assert(pet.restore(s));
    pet.setMood(80); pet.setSatiety(50); pet.setCleanliness(50);
    return pet;
}
static const char* parse(const char* cmd, const char* json, Ble::LifecycleRequest& out) {
    StaticJsonDocument<512> d; assert(!deserializeJson(d, json));
    LifecycleAction action;
    assert(Ble::isLifecycleCommand(cmd, action));
    return Ble::parseLifecycleCommand(d.as<JsonVariantConst>(), 40, action, out);
}
static void result(uint32_t id, uint64_t op, LifecycleAction a, const Ble::LifecycleOutcome& o, char (&json)[1025]) {
    uint16_t len = 0;
    assert(Ble::makeLifecycleResultJson(id, op, a, o, json, sizeof(json), len) && len == strlen(json));
}

static void parsing() {
    LifecycleAction a;
    assert(Ble::isLifecycleCommand("depart", a) && a == LifecycleAction::Depart);
    assert(Ble::isLifecycleCommand("adopt", a) && a == LifecycleAction::Adopt);
    assert(!Ble::isLifecycleCommand("Depart", a) && !Ble::isLifecycleCommand("care", a) && !Ble::isLifecycleCommand(nullptr, a));
    assert(strcmp(Ble::lifecycleActionName(LifecycleAction::Depart), "depart") == 0 &&
           strcmp(Ble::lifecycleActionName(LifecycleAction::Adopt), "adopt") == 0);
    Ble::LifecycleRequest r;
    assert(strcmp(parse("depart", R"({"pet_id":"42","op":"9f2c41d07ab3e815"})", r), "ok") == 0);
    assert(r.commandId == 40 && r.action == LifecycleAction::Depart && r.petId == 42 && r.op == 0x9f2c41d07ab3e815ULL);
    assert(strcmp(parse("adopt", R"({"pet_id":"18446744073709551615","op":"0000000000000001"})", r), "ok") == 0);
    assert(r.action == LifecycleAction::Adopt && r.petId == UINT64_MAX && r.op == 1);
    // An action field is ignored; missing or wrongly typed fields, then format errors.
    assert(strcmp(parse("adopt", R"({"action":"feed","pet_id":"7","op":"0000000000000002"})", r), "ok") == 0);
    assert(strcmp(parse("depart", R"({"pet_id":"42"})", r), "missing_field") == 0);
    assert(strcmp(parse("depart", R"({"op":"9f2c41d07ab3e815"})", r), "missing_field") == 0);
    assert(strcmp(parse("adopt", R"({"pet_id":42,"op":"9f2c41d07ab3e815"})", r), "missing_field") == 0);
    assert(strcmp(parse("adopt", R"({"pet_id":"42","op":7})", r), "missing_field") == 0);
    assert(strcmp(parse("depart", R"({"pet_id":"42","op":"9F2C41D07AB3E815"})", r), "invalid_value") == 0);
    assert(strcmp(parse("depart", R"({"pet_id":"42","op":"9f2c"})", r), "invalid_value") == 0);
    assert(strcmp(parse("adopt", R"({"pet_id":"-1","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
    assert(strcmp(parse("adopt", R"({"pet_id":"","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
}

static void departRules() {
    using Ble::departEligibility;
    auto pet = living(42, Pet::LifeStage::Adult);
    assert(strcmp(departEligibility(42, pet, "idle", 0), "ok") == 0);
    assert(strcmp(departEligibility(42, pet, "menu", kLimit - 1), "ok") == 0);
    const char* busy[] = {"sleeping", "ended", "device_game", "web_game", "busy", "future_value"};
    for (const char* activity : busy) assert(strcmp(departEligibility(42, pet, activity, 0), "busy") == 0);
    assert(strcmp(departEligibility(42, pet, nullptr, 0), "busy") == 0);
    // Order: pet_changed, not_allowed, memorial_full, busy.
    assert(strcmp(departEligibility(42, pet, "idle", kLimit), "memorial_full") == 0);
    assert(strcmp(departEligibility(42, pet, "busy", kLimit + 1), "memorial_full") == 0);
    assert(strcmp(departEligibility(43, pet, "busy", kLimit), "pet_changed") == 0);
    auto baby = living(7, Pet::LifeStage::Baby);
    assert(strcmp(departEligibility(7, baby, "menu", 3), "ok") == 0);
    baby.setSick(true);  // Sick pets may be sent off.
    assert(strcmp(departEligibility(7, baby, "idle", 3), "ok") == 0);
    auto egg = living(5, Pet::LifeStage::Egg);
    assert(strcmp(departEligibility(5, egg, "idle", 0), "not_allowed") == 0);
    assert(strcmp(departEligibility(5, egg, "busy", kLimit), "not_allowed") == 0);
    auto sleeper = living(6, Pet::LifeStage::Adult); assert(sleeper.beginNormalSleep());
    assert(strcmp(departEligibility(6, sleeper, "sleeping", 0), "not_allowed") == 0);
    auto gone = living(8, Pet::LifeStage::Adult); assert(gone.depart());
    assert(strcmp(departEligibility(8, gone, "ended", 0), "not_allowed") == 0);
    auto dead = living(9, Pet::LifeStage::Adult); dead.setSick(true); dead.setDead(true);
    assert(strcmp(departEligibility(9, dead, "idle", kLimit), "not_allowed") == 0);
}

static void adoptRules() {
    using Ble::adoptEligibility;
    auto dead = living(42, Pet::LifeStage::Adult); dead.setSick(true); dead.setDead(true); assert(dead.isDead());
    auto gone = living(43, Pet::LifeStage::Baby); assert(gone.depart());
    assert(strcmp(adoptEligibility(42, dead, "ended", 5, true), "ok") == 0);
    assert(strcmp(adoptEligibility(43, gone, "ended", kLimit, true), "ok") == 0);  // Exactly the limit.
    // The overflow slot in use: memorial_full, ahead of busy.
    assert(strcmp(adoptEligibility(42, dead, "ended", kLimit + 1, true), "memorial_full") == 0);
    assert(strcmp(adoptEligibility(42, dead, "busy", kLimit + 1, false), "memorial_full") == 0);
    // Not ended yet, or unsaved memorial, or not the end screens.
    const char* busy[] = {"idle", "menu", "sleeping", "device_game", "web_game", "busy", "future_value"};
    for (const char* activity : busy) assert(strcmp(adoptEligibility(42, dead, activity, 1, true), "busy") == 0);
    assert(strcmp(adoptEligibility(42, dead, "ended", 1, false), "busy") == 0);
    assert(strcmp(adoptEligibility(42, dead, nullptr, 1, true), "busy") == 0);
    auto alive = living(44, Pet::LifeStage::Adult);
    assert(strcmp(adoptEligibility(44, alive, "idle", 0, false), "not_allowed") == 0);
    assert(strcmp(adoptEligibility(44, alive, "busy", kLimit + 1, false), "not_allowed") == 0);
    auto egg = living(45, Pet::LifeStage::Egg);
    assert(strcmp(adoptEligibility(45, egg, "idle", 0, false), "not_allowed") == 0);
    // The new egg after adoption: the old id is pet_changed (a replay is answered by the op cache first).
    assert(strcmp(adoptEligibility(42, egg, "idle", 0, false), "pet_changed") == 0);
    assert(strcmp(adoptEligibility(41, dead, "ended", kLimit + 1, true), "pet_changed") == 0);
}

static void opCache() {
    const uint8_t depart = static_cast<uint8_t>(LifecycleAction::Depart), adopt = static_cast<uint8_t>(LifecycleAction::Adopt);
    const uint8_t feed = static_cast<uint8_t>(Ble::CareAction::Feed);
    Ble::OpCache cache;
    // Adopt of pet 42 accepted and done: the new pet 43 is current now.
    assert(cache.check(5, Ble::OpKind::Lifecycle, adopt, 42, 40) == Match::New);
    cache.begin(5, Ble::OpKind::Lifecycle, adopt, 42, 40);
    assert(cache.check(5, Ble::OpKind::Lifecycle, adopt, 42, 41) == Match::Pending);
    auto egg = living(43, Pet::LifeStage::Egg);
    const Ble::OpCache::Entry* e = cache.complete(5, Ble::adoptDone(egg));
    assert(e && e->done && e->commandId == 41 && e->kind == Ble::OpKind::Lifecycle && e->lifecycle.newPetId == 43);
    // Replay with the old pet id after adoption: done, not pet_changed, with the stored result.
    assert(cache.check(5, Ble::OpKind::Lifecycle, adopt, 42, 50) == Match::Done && cache.find(5)->commandId == 50);
    char first[1025], again[1025];
    result(41, 5, LifecycleAction::Adopt, e->lifecycle, first);
    result(50, 5, LifecycleAction::Adopt, cache.find(5)->lifecycle, again);
    assert(strstr(first, "\"id\":41,") && strstr(again, "\"id\":50,"));
    assert(strcmp(strstr(first, "\"op\""), strstr(again, "\"op\"")) == 0 && strstr(again, R"("new_pet_id":"43")"));
    // The same op as another command, action or target is a conflict.
    assert(cache.check(5, Ble::OpKind::Lifecycle, depart, 42, 51) == Match::Conflict);
    assert(cache.check(5, Ble::OpKind::Lifecycle, adopt, 43, 51) == Match::Conflict);
    assert(cache.check(5, Ble::OpKind::Care, adopt, 42, 51) == Match::Conflict);  // Same action number, other command.
    cache.begin(6, Ble::OpKind::Care, feed, 42, 52);
    assert(cache.check(6, Ble::OpKind::Lifecycle, depart, 42, 53) == Match::Conflict);
    // A failed depart is replayed as failed.
    cache.begin(7, Ble::OpKind::Lifecycle, depart, 42, 54);
    cache.complete(7, Ble::lifecycleFailed("save_failed"));
    assert(cache.check(7, Ble::OpKind::Lifecycle, depart, 42, 55) == Match::Done);
    assert(!cache.find(7)->lifecycle.done && strcmp(cache.find(7)->lifecycle.code, "save_failed") == 0);
}

static void actionResults() {
    char json[1025];
    result(40, 0x9f2c41d07ab3e815ULL, LifecycleAction::Depart, Ble::departDone(true), json);
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":40,"op":"9f2c41d07ab3e815","action":"depart","outcome":"done","changes":{"memorial_saved":true}})") == 0);
    result(40, 1, LifecycleAction::Depart, Ble::departDone(false), json);
    assert(strstr(json, R"("action":"depart","outcome":"done","changes":{"memorial_saved":false}})"));
    Pet::PetData egg; assert(egg.startNewEgg(43, "Kororo", Pet::SpeciesId::Wyvern));
    const Ble::LifecycleOutcome adopted = Ble::adoptDone(egg);
    result(41, 2, LifecycleAction::Adopt, adopted, json);
    char expected[256];
    snprintf(expected, sizeof(expected), R"({"v":1,"type":"action_result","id":41,"op":"0000000000000002","action":"adopt","outcome":"done","changes":{"new_pet_id":"43","name":"Kororo","species_id":%u}})",
             static_cast<unsigned>(Pet::SpeciesId::Wyvern));
    assert(strcmp(json, expected) == 0);
    for (LifecycleAction a : {LifecycleAction::Depart, LifecycleAction::Adopt}) {
        result(42, 3, a, Ble::lifecycleFailed("save_failed"), json);
        snprintf(expected, sizeof(expected), R"({"v":1,"type":"action_result","id":42,"op":"0000000000000003","action":"%s","outcome":"failed","code":"save_failed"})",
                 Ble::lifecycleActionName(a));
        assert(strcmp(json, expected) == 0);
    }
    // Longest adopt result: maximum ids and an 8-letter name.
    Pet::PetData big; assert(big.startNewEgg(UINT64_MAX, "Abcdefgh", Pet::SpeciesId::Bird));
    uint16_t len = 0;
    assert(Ble::makeLifecycleResultJson(UINT32_MAX, UINT64_MAX, LifecycleAction::Adopt, Ble::adoptDone(big), json, sizeof(json), len));
    assert(len < 200 && strstr(json, R"("new_pet_id":"18446744073709551615","name":"Abcdefgh")"));
    printf("adopt action_result worst case: %u bytes\n", len);
}

int main() {
    parsing();
    departRules();
    adoptRules();
    opCache();
    actionResults();
    puts("PASS: depart/adopt parsing, eligibility order with memorial_full, lifecycle op replay after adoption and action_result JSON.");
}
