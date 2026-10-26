// Pet sync phase 4 without NimBLE: get_memorials parsing and paging, item JSON (unknown stage,
// departed, protected), worst-case page size, page hash = status hash, delete_memorial parsing,
// eligibility order, ops in the shared cache (replay after deletion) and action_result JSON.
#include <ArduinoJson.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble/CareCommand.h"
#include "ble/MemorialCommand.h"
#include "ble/Messages.h"
#include "ble/OpCache.h"
#include "ble/PetEvents.h"
#include "pet/PetData.h"
#include "storage/Memorials.h"

using Match = Ble::OpCache::Match;

static Pet::PetData living(uint64_t id) {
    Pet::PetData pet; assert(pet.startNewEgg(id, "Tamama", Pet::SpeciesId::Bird));
    auto s = pet.snapshot(); s.lifeStage = Pet::LifeStage::Adult; s.ageSeconds = Pet::PetData::kAdultAgeSeconds;
    assert(pet.restore(s));
    pet.setMood(80); pet.setSatiety(50); pet.setCleanliness(50);
    return pet;
}
static Storage::MemorialRecord record(uint64_t id, const char* name = "Old",
                                      Storage::FarewellKind kind = Storage::FarewellKind::Resting,
                                      uint8_t stage = static_cast<uint8_t>(Pet::LifeStage::Adult)) {
    Storage::MemorialRecord r{}; r.petId = id; strcpy(r.name, name); r.speciesId = Pet::SpeciesId::Bird;
    r.ageSeconds = id * 60; r.kind = kind; r.stage = stage;
    return r;
}
static const char* offsetOf(const char* json, uint32_t& offset) {
    StaticJsonDocument<256> d; assert(!deserializeJson(d, json));
    return Ble::parseGetMemorials(d.as<JsonVariantConst>(), offset);
}
static const char* deleteOf(const char* json, Ble::MemorialRequest& r) {
    StaticJsonDocument<256> d; assert(!deserializeJson(d, json));
    return Ble::parseDeleteMemorial(d.as<JsonVariantConst>(), 31, r);
}
// The page as parsed JSON; the text stays in json.
static void page(const Storage::Memorials& m, const Pet::PetData& pet, uint32_t offset,
                 char (&json)[1025], DynamicJsonDocument& d) {
    uint16_t len = 0;
    assert(Ble::makeMemorialsPageJson(30, m, pet, offset, json, sizeof(json), len) && len == strlen(json));
    assert(!deserializeJson(d, static_cast<const char*>(json)));  // Copy: json stays readable.
    assert(d["v"] == 1 && strcmp(d["type"], "memorials") == 0 && d["id"] == 30);
    assert(d["total"] == m.count() && d["offset"] == offset);
}

static void parsing() {
    uint32_t o = 99;
    assert(strcmp(offsetOf(R"({"offset":0})", o), "ok") == 0 && o == 0);
    assert(strcmp(offsetOf(R"({"offset":5})", o), "ok") == 0 && o == 5);
    assert(strcmp(offsetOf(R"({"offset":4294967295})", o), "ok") == 0 && o == UINT32_MAX);
    // Beyond 32 bits (and beyond 64 bits signed): an empty page, not an error.
    assert(strcmp(offsetOf(R"({"offset":4294967296})", o), "ok") == 0 && o == UINT32_MAX);
    o = 0; assert(strcmp(offsetOf(R"({"offset":18446744073709551615})", o), "ok") == 0 && o == UINT32_MAX);
    assert(strcmp(offsetOf(R"({"offset":-1})", o), "invalid_value") == 0);
    assert(strcmp(offsetOf(R"({"offset":-9223372036854775808})", o), "invalid_value") == 0);
    const char* wrong[] = {R"({})", R"({"offset":"0"})", R"({"offset":1.5})", R"({"offset":true})",
                           R"({"offset":null})", R"({"offset":[0]})"};
    for (const char* json : wrong) assert(strcmp(offsetOf(json, o), "missing_field") == 0);

    Ble::MemorialRequest r;
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"3","op":"9f2c41d07ab3e815"})", r), "ok") == 0);
    assert(r.commandId == 31 && r.memorialPetId == 3 && r.op == 0x9f2c41d07ab3e815ULL);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"18446744073709551615","op":"0000000000000001"})", r), "ok") == 0 &&
           r.memorialPetId == UINT64_MAX && r.op == 1);
    // pet_id is not the field: missing; wrong types missing; formats invalid.
    assert(strcmp(deleteOf(R"({"pet_id":"3","op":"9f2c41d07ab3e815"})", r), "missing_field") == 0);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"3"})", r), "missing_field") == 0);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":3,"op":"9f2c41d07ab3e815"})", r), "missing_field") == 0);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"3","op":"9F2C41D07AB3E815"})", r), "invalid_value") == 0);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"-3","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
    assert(strcmp(deleteOf(R"({"memorial_pet_id":"18446744073709551616","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
}

static void paging() {
    Storage::Memorials m;
    for (uint64_t i = 1; i <= 7; ++i) assert(m.append(record(i)));
    const auto pet = living(42);
    char json[1025]; DynamicJsonDocument d(4096);
    const struct { uint32_t offset; uint8_t size; uint64_t first; } pages[] = {
        {0, 5, 1}, {5, 2, 6}, {2, 5, 3}, {6, 1, 7}, {7, 0, 0}, {8, 0, 0}, {UINT32_MAX, 0, 0},
    };
    for (const auto& p : pages) {
        page(m, pet, p.offset, json, d);
        JsonArray items = d["items"];
        assert(!items.isNull() && items.size() == p.size);
        for (uint8_t i = 0; i < p.size; ++i) {
            char id[21]; snprintf(id, sizeof(id), "%llu", static_cast<unsigned long long>(p.first + i));
            assert(strcmp(items[i]["pet_id"], id) == 0);  // Storage order.
        }
    }
    page(m, pet, 7, json, d); assert(strstr(json, R"("items":[])"));
    // An empty archive: total 0, hash 00000000, empty items.
    Storage::Memorials empty;
    page(empty, pet, 0, json, d);
    assert(strcmp(d["hash"], "00000000") == 0 && d["items"].as<JsonArray>().size() == 0);
    // Exactly 5 and the full archive with the overflow slot.
    Storage::Memorials five; for (uint64_t i = 1; i <= 5; ++i) assert(five.append(record(i)));
    page(five, pet, 0, json, d); assert(d["items"].as<JsonArray>().size() == 5);
    page(five, pet, 5, json, d); assert(d["items"].as<JsonArray>().size() == 0);
    Storage::Memorials full; for (uint64_t i = 1; i <= Storage::kMemorialCapacity; ++i) assert(full.append(record(i)));
    page(full, pet, 30, json, d); assert(d["total"] == 33 && d["items"].as<JsonArray>().size() == 3);
    assert(strcmp(d["items"][2]["pet_id"], "33") == 0);
}

static void items() {
    Storage::Memorials m;
    Storage::MemorialRecord old = record(3, "Tamama"); old.stage = Storage::kUnknownMemorialStage; old.ageSeconds = 3600;
    assert(m.append(old));
    Storage::MemorialRecord baby = record(5, "Kororo", Storage::FarewellKind::Departed, static_cast<uint8_t>(Pet::LifeStage::Baby));
    baby.speciesId = Pet::SpeciesId::Wyvern; baby.ageSeconds = UINT64_MAX;
    assert(m.append(baby));
    assert(m.append(record(9, "Q\"\\x")));
    char json[1025]; DynamicJsonDocument d(4096);
    auto pet = living(9);
    page(m, pet, 0, json, d);
    assert(strstr(json, R"({"pet_id":"3","name":"Tamama","species_id":1,"age_seconds":"3600","kind":"resting","stage":"unknown","protected":false})"));
    assert(strstr(json, R"({"pet_id":"5","name":"Kororo","species_id":2,"age_seconds":"18446744073709551615","kind":"departed","stage":"baby","protected":false})"));
    // Escaped name round trip; pet 9 is alive, so its (test) record is not protected.
    assert(strcmp(d["items"][2]["name"], "Q\"\\x") == 0 && d["items"][2]["stage"] == "adult" && d["items"][2]["protected"] == false);
    // The current pet ended: its own record is protected, the others are not.
    pet.setSick(true); pet.setDead(true); assert(pet.isEnded());
    page(m, pet, 0, json, d);
    assert(d["items"][2]["protected"] == true && d["items"][0]["protected"] == false && d["items"][1]["protected"] == false);
    auto gone = living(5); assert(gone.depart());
    page(m, gone, 0, json, d);
    assert(d["items"][1]["protected"] == true && d["items"][2]["protected"] == false);
    // An ended pet whose record is not saved yet protects nothing; a new egg (adopted) neither.
    auto other = living(11); assert(other.depart());
    page(m, other, 0, json, d);
    for (JsonObject item : d["items"].as<JsonArray>()) assert(item["protected"] == false);
    // The egg stage name exists for completeness; an invalid record never reaches the archive.
    Storage::Memorials eggs; Storage::MemorialRecord egg = record(4); egg.stage = 0;
    assert(!eggs.append(egg));
}

// Page hash equals status memorial_hash, and changes with the archive.
static void hash() {
    Storage::Memorials m;
    for (uint64_t i = 1; i <= 6; ++i) assert(m.append(record(i)));
    const auto pet = living(42);
    char status[1025], json[1025]; uint16_t len = 0; DynamicJsonDocument d(4096), s(4096);
    Ble::StatusSources sources; sources.memorials = &m;
    assert(Ble::makeStatusJson(pet, "esp32-AABBCCDDEEFF", "idle", sources, status, sizeof(status), len));
    assert(!deserializeJson(s, static_cast<const char*>(status)));
    for (uint32_t offset : {0u, 5u, 6u}) {
        page(m, pet, offset, json, d);
        assert(strcmp(d["hash"], s["memorial_hash"]) == 0 && strlen(d["hash"]) == 8);
    }
    char before[9]; strcpy(before, d["hash"]);
    assert(m.remove(2));
    page(m, pet, 0, json, d);
    assert(strcmp(d["hash"], before) != 0 && d["total"] == 5);
    assert(Ble::makeStatusJson(pet, "esp32-AABBCCDDEEFF", "idle", sources, status, sizeof(status), len));
    assert(!deserializeJson(s, static_cast<const char*>(status)) && strcmp(d["hash"], s["memorial_hash"]) == 0 && s["memorial_count"] == 5);
}

// Five records with the longest ids, ages and (escaped) names, the largest id/offset/total.
static void worstCase() {
    unsigned worst = 0;
    for (int variant = 0; variant < 2; ++variant) {
        Storage::Memorials m;
        for (uint8_t i = 0; i < Storage::kMemorialCapacity; ++i) {
            Storage::MemorialRecord r = variant ?
                record(UINT64_MAX - i, "\"\"\"\"\"\"\"\"", Storage::FarewellKind::Resting, Storage::kUnknownMemorialStage) :
                record(UINT64_MAX - i, "\\\\\\\\\\\\\\\\", Storage::FarewellKind::Departed, static_cast<uint8_t>(Pet::LifeStage::Adult));
            r.speciesId = Pet::SpeciesId::Wyvern; r.ageSeconds = UINT64_MAX - i;
            assert(m.append(r));
        }
        const auto pet = living(1);
        char json[1025]; uint16_t len = 0;
        for (uint32_t offset = 0; offset < m.count(); offset += Ble::kMemorialPageSize) {
            assert(Ble::makeMemorialsPageJson(UINT32_MAX, m, pet, offset, json, sizeof(json), len));
            if (len > worst) worst = len;
        }
        assert(Ble::makeMemorialsPageJson(UINT32_MAX, m, pet, UINT32_MAX, json, sizeof(json), len));
    }
    printf("memorials page worst case: %u bytes\n", worst);
    assert(worst <= Ble::kMaxMessageBytes);
}

static void eligibility() {
    using Ble::deleteMemorialEligibility;
    Storage::Memorials m;
    for (uint64_t i = 1; i <= 3; ++i) assert(m.append(record(i)));
    auto pet = living(42);
    const char* allowed[] = {"idle", "menu", "sleeping", "ended"};
    for (const char* a : allowed) assert(strcmp(deleteMemorialEligibility(2, pet, m, a), "ok") == 0);
    const char* busy[] = {"device_game", "web_game", "busy", "future_value"};
    for (const char* a : busy) assert(strcmp(deleteMemorialEligibility(2, pet, m, a), "busy") == 0);
    assert(strcmp(deleteMemorialEligibility(2, pet, m, nullptr), "busy") == 0);
    // Order: not_found, then not_allowed (protected), then busy.
    assert(strcmp(deleteMemorialEligibility(4, pet, m, "busy"), "not_found") == 0);
    assert(strcmp(deleteMemorialEligibility(0, pet, m, "idle"), "not_found") == 0);
    Storage::Memorials none; assert(strcmp(deleteMemorialEligibility(1, pet, none, "idle"), "not_found") == 0);
    auto dead = living(3); dead.setSick(true); dead.setDead(true);
    assert(strcmp(deleteMemorialEligibility(3, dead, m, "ended"), "not_allowed") == 0);
    assert(strcmp(deleteMemorialEligibility(3, dead, m, "busy"), "not_allowed") == 0);
    assert(strcmp(deleteMemorialEligibility(1, dead, m, "ended"), "ok") == 0);
    assert(strcmp(deleteMemorialEligibility(9, dead, m, "busy"), "not_found") == 0);
    auto gone = living(1); assert(gone.depart());
    assert(strcmp(deleteMemorialEligibility(1, gone, m, "ended"), "not_allowed") == 0);
    // A living pet whose id matches a record (rebuilt archive) is not protected.
    auto same = living(2);
    assert(strcmp(deleteMemorialEligibility(2, same, m, "idle"), "ok") == 0);
    assert(Ble::findMemorial(m, 3) == 2 && Ble::findMemorial(m, 1) == 0 && Ble::findMemorial(m, 7) == -1);
}

static void opsAndResults() {
    using Ble::OpKind;
    Ble::OpCache cache;
    const uint8_t feed = static_cast<uint8_t>(Ble::CareAction::Feed);
    // Accepted, pending, then done: a replay after the deletion gets the stored result, not not_found.
    assert(cache.check(5, OpKind::Memorial, 0, 3, 31) == Match::New);
    cache.begin(5, OpKind::Memorial, 0, 3, 31);
    assert(cache.check(5, OpKind::Memorial, 0, 3, 32) == Match::Pending);
    const Ble::OpCache::Entry* e = cache.complete(5, Ble::memorialDone());
    assert(e && e->done && e->kind == OpKind::Memorial && e->memorial.done && e->commandId == 32);
    assert(cache.check(5, OpKind::Memorial, 0, 3, 40) == Match::Done && cache.find(5)->commandId == 40);
    char first[1025], again[1025]; uint16_t len = 0;
    assert(Ble::makeMemorialResultJson(32, 5, e->memorial, first, sizeof(first), len) && len == strlen(first));
    assert(Ble::makeMemorialResultJson(40, 5, cache.find(5)->memorial, again, sizeof(again), len));
    assert(strcmp(first, R"({"v":1,"type":"action_result","id":32,"op":"0000000000000005","action":"delete_memorial","outcome":"done","changes":{}})") == 0);
    assert(strstr(again, "\"id\":40,") && strcmp(strstr(first, "\"op\""), strstr(again, "\"op\"")) == 0);
    // Another target, another command or the same op as care: conflicts.
    assert(cache.check(5, OpKind::Memorial, 0, 4, 41) == Match::Conflict);
    assert(cache.check(5, OpKind::Care, 0, 3, 41) == Match::Conflict);
    assert(cache.check(5, OpKind::Lifecycle, 0, 3, 41) == Match::Conflict);
    cache.begin(6, OpKind::Care, feed, 3, 42);
    assert(cache.check(6, OpKind::Memorial, 0, 3, 43) == Match::Conflict);
    // A failed deletion replays as failed.
    cache.begin(7, OpKind::Memorial, 0, 2, 44);
    cache.complete(7, Ble::memorialFailed("save_failed"));
    assert(cache.check(7, OpKind::Memorial, 0, 2, 45) == Match::Done);
    assert(Ble::makeMemorialResultJson(45, 7, cache.find(7)->memorial, again, sizeof(again), len));
    assert(strcmp(again, R"({"v":1,"type":"action_result","id":45,"op":"0000000000000007","action":"delete_memorial","outcome":"failed","code":"save_failed"})") == 0);
    // A null code still reads save_failed.
    assert(Ble::makeMemorialResultJson(1, 1, Ble::memorialFailed(nullptr), again, sizeof(again), len) &&
           strstr(again, R"("code":"save_failed")"));
}

// memorial_deleted: source web after markFromWeb, device otherwise, with memorial_pet_id.
static void events() {
    Storage::Memorials m;
    for (uint64_t i = 1; i <= 3; ++i) assert(m.append(record(i)));
    const auto pet = living(42);
    Ble::PetEventDetector detector; detector.reset(pet, m);
    Ble::PetEvent ev[8];
    assert(m.remove(1));
    detector.markFromWeb(Ble::PetEventType::MemorialDeleted);
    assert(detector.detect(pet, m, ev, 8) == 1 && ev[0].type == Ble::PetEventType::MemorialDeleted &&
           ev[0].memorialPetId == 2 && ev[0].fromWeb);
    char json[1025]; uint16_t len = 0;
    assert(Ble::makePetEventJson(ev[0], json, sizeof(json), len));
    assert(strcmp(json, R"({"v":1,"type":"pet_event","pet_id":"42","event":"memorial_deleted","memorial_pet_id":"2","source":"web"})") == 0);
    assert(m.remove(0));  // The joystick path: no mark.
    assert(detector.detect(pet, m, ev, 8) == 1 && ev[0].memorialPetId == 1 && !ev[0].fromWeb);
    assert(Ble::makePetEventJson(ev[0], json, sizeof(json), len) && strstr(json, R"("source":"device")"));
}

static void deviceInfo() {
    char json[1025]; uint16_t len = 0;
    assert(Ble::makeDeviceInfoJson(7, "ESP32-PET", "esp32-AABBCCDDEEFF", json, sizeof(json), len));
    assert(strstr(json, R"("features":["web_game","pet_events","care","lifecycle","memorials","sound"])"));
}

int main() {
    parsing(); paging(); items(); hash(); worstCase(); eligibility(); opsAndResults(); events(); deviceInfo();
    puts("PASS: get_memorials offsets and pages, item fields and protection, page hash = status hash, worst-case size, delete_memorial parsing, eligibility order, op replay after deletion, results and event source.");
}
