// Pet sync phase 1 without NimBLE: sad hysteresis, memorial hash, status / device_info JSON,
// pet_event messages, state-event detection and the event queue admission rule.
#include <ArduinoJson.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble/Messages.h"
#include "ble/PetEvents.h"
#include "pet/PetData.h"
#include "pet/SadState.h"
#include "storage/Memorials.h"


static Pet::PetData living(uint64_t id, const char* name, Pet::LifeStage stage) {
    Pet::PetData pet; assert(pet.startNewEgg(id, name, Pet::SpeciesId::Bird));
    auto s = pet.snapshot(); s.lifeStage = stage;
    s.ageSeconds = stage == Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds :
        stage == Pet::LifeStage::Adult ? Pet::PetData::kAdultAgeSeconds : 0;
    assert(pet.restore(s)); return pet;
}
static Storage::MemorialRecord record(uint64_t id, const char* name, uint64_t age = 3600) {
    Storage::MemorialRecord r{}; r.petId = id; strcpy(r.name, name);
    r.speciesId = Pet::SpeciesId::Bird; r.ageSeconds = age;
    r.kind = Storage::FarewellKind::Resting; r.stage = static_cast<uint8_t>(Pet::LifeStage::Adult);
    return r;
}
static void needs(Pet::PetData& pet, int mood, int satiety, int clean) {
    pet.setMood(mood); pet.setSatiety(satiety); pet.setCleanliness(clean);
}

static void sadHysteresis() {
    auto pet = living(5, "Tamama", Pet::LifeStage::Adult);
    Pet::SadState sad; needs(pet, 80, 80, 80);
    assert(sad.update(pet));  // First observation adopts the pet id.
    assert(!sad.any() && sad.petId() == 5);
    // Mood: enter at 40, stay through 44, leave at 45.
    pet.setMood(41); sad.update(pet); assert(!sad.mood());
    pet.setMood(40); sad.update(pet); assert(sad.mood() && sad.any());
    pet.setMood(44); sad.update(pet); assert(sad.mood());
    pet.setMood(45); sad.update(pet); assert(!sad.mood());
    pet.setMood(44); sad.update(pet); assert(!sad.mood());  // Not re-entered above 40.
    // Satiety and cleanliness: enter at 25, stay through 29, leave at 30.
    for (int which = 0; which < 2; ++which) {
        const auto set = [&](int v) { if (which) pet.setCleanliness(v); else pet.setSatiety(v); };
        const auto on = [&] { return which ? sad.cleanliness() : sad.satiety(); };
        set(26); sad.update(pet); assert(!on());
        set(25); sad.update(pet); assert(on());
        set(29); sad.update(pet); assert(on());
        set(30); sad.update(pet); assert(!on());
    }
    // Sickness has no buffer.
    pet.setSick(true); sad.update(pet); assert(sad.sick() && sad.any());
    pet.setSick(false); sad.update(pet); assert(!sad.sick() && !sad.any());
    // A buffered reason is cleared by a new pet id even at the same values.
    pet.setMood(40); sad.update(pet); pet.setMood(43); sad.update(pet); assert(sad.mood());
    auto other = living(6, "Kororo", Pet::LifeStage::Adult); needs(other, 43, 80, 80);
    assert(sad.update(other) && !sad.mood() && sad.petId() == 6);
    // Eggs and ended pets are never sad.
    auto egg = living(7, "Egg", Pet::LifeStage::Egg); needs(egg, 0, 0, 0);
    sad.update(egg); assert(!sad.any());
    auto gone = living(8, "Gone", Pet::LifeStage::Baby); needs(gone, 0, 0, 0); gone.setSick(true);
    sad.update(gone); assert(sad.any());
    assert(gone.depart()); sad.update(gone); assert(!sad.any());
}

static void memorialHash() {
    Storage::Memorials a, b;
    assert(Storage::memorialHash(a) == 0);
    assert(a.append(record(3, "Tamama")) && a.append(record(9, "Kororo", 7200)));
    assert(b.append(record(3, "Tamama")) && b.append(record(9, "Kororo", 7200)));
    const uint32_t h = Storage::memorialHash(a);
    assert(h != 0 && h == Storage::memorialHash(b) && h == Storage::memorialHash(a));
    // Fixed vector (zlib.crc32 of the 27 bytes) pins the byte encoding (petId LE, name padded to 8, species, age LE, kind, stage),
    // so the value cannot drift with struct padding or a compiler change.
    Storage::Memorials one; assert(one.append(record(3, "Tamama")));
    assert(Storage::memorialHash(one) == 0x1620df97UL);
    // Every field and the order count.
    Storage::Memorials c; assert(c.append(record(9, "Kororo", 7200)) && c.append(record(3, "Tamama")));
    assert(Storage::memorialHash(c) != h);
    Storage::Memorials d; assert(d.append(record(3, "Tamama")) && d.append(record(9, "Kororo", 7201)));
    assert(Storage::memorialHash(d) != h);
    Storage::Memorials e; auto departed = record(9, "Kororo", 7200); departed.kind = Storage::FarewellKind::Departed;
    assert(e.append(record(3, "Tamama")) && e.append(departed) && Storage::memorialHash(e) != h);
    Storage::Memorials f; auto wyvern = record(9, "Kororo", 7200); wyvern.speciesId = Pet::SpeciesId::Wyvern;
    assert(f.append(record(3, "Tamama")) && f.append(wyvern) && Storage::memorialHash(f) != h);
    // Deleting and re-adding the same record restores the value.
    assert(a.remove(1) && Storage::memorialHash(a) != h);
    assert(a.append(record(9, "Kororo", 7200)) && Storage::memorialHash(a) == h);
}

static void statusJson() {
    char json[1025]; uint16_t len = 0;
    auto pet = living(42, "Tamama", Pet::LifeStage::Adult); needs(pet, 40, 20, 80);
    Pet::SadState sad; sad.update(pet);
    Storage::Memorials archive; assert(archive.append(record(3, "Tamama")));
    Ble::StatusSources sources; sources.sad = &sad; sources.memorials = &archive;
    assert(Ble::makeStatusJson(pet, "esp32-AABBCCDDEEFF", "menu", sources, json, sizeof(json), len));
    assert(len == strlen(json));
    StaticJsonDocument<2048> d; assert(!deserializeJson(d, json));
    assert(strcmp(d["type"], "status") == 0 && strcmp(d["activity"], "menu") == 0);
    assert(strcmp(d["pet"]["id"], "42") == 0 && strcmp(d["pet"]["name"], "Tamama") == 0);
    JsonArray reasons = d["pet"]["sad_reasons"];
    assert(reasons.size() == 2 && strcmp(reasons[0], "mood") == 0 && strcmp(reasons[1], "satiety") == 0);
    assert(d["pet"]["danger_seconds"] == 0 && d["pet"]["sick_seconds"] == 0);
    assert(!d["pet"].containsKey("death_cause"));
    assert(d["memorial_count"] == 1 && strcmp(d["memorial_hash"], "1620df97") == 0);
    assert(d["pet"]["level"].is<int>() && d["pet"]["exp_to_next_level"].is<int>());
    // Old shape without sources: no sync fields except the always-present seconds.
    assert(Ble::makeStatusJson(pet, "esp32-AABBCCDDEEFF", "idle", Ble::StatusSources{}, json, sizeof(json), len));
    d.clear(); assert(!deserializeJson(d, json));
    assert(!d["pet"].containsKey("sad_reasons") && !d.containsKey("memorial_count") && !d.containsKey("memorial_hash"));
    // A stale observation of another pet reports no reasons.
    auto other = living(43, "Kororo", Pet::LifeStage::Adult); needs(other, 10, 10, 10);
    assert(Ble::makeStatusJson(other, "esp32-AABBCCDDEEFF", "idle", sources, json, sizeof(json), len));
    d.clear(); assert(!deserializeJson(d, json)); assert(d["pet"]["sad_reasons"].as<JsonArray>().size() == 0);
    // Danger and sickness seconds; death cause only once dead.
    auto sick = living(44, "Sick", Pet::LifeStage::Adult); needs(sick, 80, 0, 80);
    sick.advanceSeconds(Pet::PetData::kSicknessExposureSeconds + 30);
    assert(sick.isSick() && sick.sickAwakeSeconds() > 0);
    assert(Ble::makeStatusJson(sick, "esp32-AABBCCDDEEFF", "idle", sources, json, sizeof(json), len));
    d.clear(); assert(!deserializeJson(d, json));
    assert(d["pet"]["danger_seconds"] == sick.dangerSeconds() && d["pet"]["sick_seconds"] == sick.sickAwakeSeconds());
    assert(!d["pet"].containsKey("death_cause"));
    // Same loop as a treatment or farewell, before the UI refreshes the buffers: the pet's own state wins.
    sad.update(sick); assert(sad.sick() && sad.satiety());
    auto treated = sick; assert(treated.treat());
    assert(Ble::makeStatusJson(treated, "esp32-AABBCCDDEEFF", "busy", sources, json, sizeof(json), len));
    d.clear(); assert(!deserializeJson(d, json));
    JsonArray afterTreat = d["pet"]["sad_reasons"];
    assert(afterTreat.size() == 1 && strcmp(afterTreat[0], "satiety") == 0);
    auto gone = sick; assert(gone.depart());
    assert(Ble::makeStatusJson(gone, "esp32-AABBCCDDEEFF", "busy", sources, json, sizeof(json), len));
    d.clear(); assert(!deserializeJson(d, json)); assert(d["pet"]["sad_reasons"].as<JsonArray>().size() == 0);
    sick.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds); assert(sick.isDead());
    sad.update(sick);
    assert(Ble::makeStatusJson(sick, "esp32-AABBCCDDEEFF", "ended", sources, json, sizeof(json), len));
    d.clear(); assert(!deserializeJson(d, json));
    assert(strcmp(d["pet"]["death_cause"], "untreated_sickness") == 0 && d["pet"]["sad_reasons"].as<JsonArray>().size() == 0);
    // Worst case: longest escaped name, 20-digit id and age, all reasons, longest activity,
    // full archive (33 records with the overflow slot).
    unsigned worst = 0;
    for (int dead = 0; dead < 2; ++dead) {
        Pet::PetData big; assert(big.startNewEgg(UINT64_MAX, "\"\\\"\\\"\\\"\\", Pet::SpeciesId::Wyvern));
        auto s = big.snapshot(); s.lifeStage = Pet::LifeStage::Adult; s.ageSeconds = UINT64_MAX - 10;
        s.level = 20; s.satiety = s.mood = s.cleanliness = 0; s.healthState = Pet::HealthState::Sick;
        s.dangerSeconds = Pet::PetData::kSicknessExposureSeconds; s.sickAwakeSeconds = Pet::PetData::kDeathAfterSickAwakeSeconds - 1;
        assert(big.restore(s));
        if (dead) { big.advanceSeconds(1); assert(big.isDead()); }
        Pet::SadState all; all.update(big); assert(dead || (all.mood() && all.satiety() && all.cleanliness() && all.sick()));
        Storage::Memorials full;
        for (uint64_t i = 0; i < Storage::kMemorialCapacity; ++i) assert(full.append(record(UINT64_MAX - i, "ABCDEFGH")));
        Ble::StatusSources src; src.sad = &all; src.memorials = &full;
        assert(Ble::makeStatusJson(big, "esp32-AABBCCDDEEFF", "device_game", src, json, sizeof(json), len));
        d.clear(); assert(!deserializeJson(d, json) && d["memorial_count"] == 33);
        if (len > worst) worst = len;
    }
    // Seconds are bounded by the save rules here; full uint32 values add at most 10 bytes,
    // a level below max at most 6 more digits for exp / exp_to_next_level.
    printf("status worst case: %u bytes (+16 slack)\n", worst);
    assert(worst + 16 <= Ble::kMaxMessageBytes);
}

static void deviceInfo() {
    char json[1025]; uint16_t len = 0;
    assert(Ble::makeDeviceInfoJson(7, "ESP32-PET", "esp32-AABBCCDDEEFF", json, sizeof(json), len));
    StaticJsonDocument<1024> d; assert(!deserializeJson(d, json));
    assert(strcmp(d["type"], "device_info") == 0 && d["id"] == 7 && d["max_message_bytes"] == 1024);
    JsonArray features = d["features"];
    assert(features.size() == 5 && strcmp(features[0], "web_game") == 0 && strcmp(features[1], "pet_events") == 0 &&
           strcmp(features[2], "care") == 0 && strcmp(features[3], "lifecycle") == 0 && strcmp(features[4], "memorials") == 0);
    assert(d["games"].as<JsonArray>().size() == 2);
    assert(d["rules"]["sick_after_seconds"] == static_cast<uint32_t>(Pet::PetData::kSicknessExposureSeconds));
    assert(d["rules"]["death_after_sick_seconds"] == static_cast<uint32_t>(Pet::PetData::kDeathAfterSickAwakeSeconds));
    assert(d["rules"]["memorial_limit"] == Storage::kMemorialLimit);
}

static void eventJson() {
    char json[1025]; uint16_t len = 0;
    Ble::PetEvent e; e.type = Ble::PetEventType::Fed; e.petId = 42;
    assert(Ble::makePetEventJson(e, json, sizeof(json), len));
    assert(strcmp(json, "{\"v\":1,\"type\":\"pet_event\",\"pet_id\":\"42\",\"event\":\"fed\",\"source\":\"device\"}") == 0);
    e.type = Ble::PetEventType::LevelUp; e.level = 7;
    assert(Ble::makePetEventJson(e, json, sizeof(json), len));
    assert(strcmp(json, "{\"v\":1,\"type\":\"pet_event\",\"pet_id\":\"42\",\"event\":\"level_up\",\"level\":7}") == 0);
    e.type = Ble::PetEventType::MemorialDeleted; e.petId = UINT64_MAX; e.memorialPetId = UINT64_MAX - 1;
    assert(Ble::makePetEventJson(e, json, sizeof(json), len) && len < 200);
    assert(strcmp(json, "{\"v\":1,\"type\":\"pet_event\",\"pet_id\":\"18446744073709551615\",\"event\":\"memorial_deleted\","
                        "\"memorial_pet_id\":\"18446744073709551614\",\"source\":\"device\"}") == 0);
    e.type = Ble::PetEventType::Hatched;
    assert(Ble::makePetEventJson(e, json, sizeof(json), len) && !strstr(json, "source") && !strstr(json, "level"));
    const char* names[] = {"fed","cleaned","treated","slept","woke","hatched","grew","level_up","got_sick","died",
                           "departed","adopted","memorial_added","memorial_deleted"};
    for (unsigned i = 0; i < 14; ++i) assert(strcmp(Ble::petEventName(static_cast<Ble::PetEventType>(i)), names[i]) == 0);
}

static void detection() {
    using T = Ble::PetEventType;
    Ble::PetEvent ev[8];
    Storage::Memorials archive;
    auto pet = living(10, "Tamama", Pet::LifeStage::Egg);
    Ble::PetEventDetector det;
    assert(det.detect(pet, archive, ev, 8) == 0);  // First call only records.
    assert(det.detect(pet, archive, ev, 8) == 0);
    pet.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
    assert(pet.lifeStage() == Pet::LifeStage::Baby);
    assert(det.detect(pet, archive, ev, 8) == 1 && ev[0].type == T::Hatched && ev[0].petId == 10);
    pet.advanceSeconds(Pet::PetData::kAdultAgeSeconds);
    uint8_t n = det.detect(pet, archive, ev, 8);
    assert(n >= 1 && ev[0].type == T::Grew);
    pet.setSatiety(100); det.detect(pet, archive, ev, 8);
    const uint8_t level = pet.level(); pet.gainExp(pet.expToNextLevel() * 2 + 300);
    n = det.detect(pet, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::LevelUp && ev[0].level == pet.level() && pet.level() > level + 1);
    pet.setSick(true);
    assert(det.detect(pet, archive, ev, 8) == 1 && ev[0].type == T::GotSick);
    pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds);
    assert(det.detect(pet, archive, ev, 8) == 1 && ev[0].type == T::Died);
    assert(archive.append(pet));
    n = det.detect(pet, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::MemorialAdded && ev[0].memorialPetId == 10 && ev[0].petId == 10);
    // Adoption: one event for the new pet, nothing diffed against the old one.
    auto egg = living(11, "Kororo", Pet::LifeStage::Egg);
    n = det.detect(egg, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::Adopted && ev[0].petId == 11);
    assert(det.detect(egg, archive, ev, 8) == 0);
    // Deletion, and a delete plus add in the same loop.
    assert(archive.remove(0));
    n = det.detect(egg, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::MemorialDeleted && ev[0].memorialPetId == 10);
    assert(archive.append(record(20, "A")) && det.detect(egg, archive, ev, 8) == 1);
    assert(archive.remove(0) && archive.append(record(21, "B")));
    n = det.detect(egg, archive, ev, 8);
    assert(n == 2 && ev[0].type == T::MemorialDeleted && ev[0].memorialPetId == 20 &&
           ev[1].type == T::MemorialAdded && ev[1].memorialPetId == 21);
    // Farewell of a living pet.
    auto baby = living(11, "Kororo", Pet::LifeStage::Baby);
    det.reset(baby, archive);
    assert(baby.depart());
    n = det.detect(baby, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::Departed && !ev[0].fromWeb);
    // A web farewell or adoption marks its events as source web for the next detection only.
    auto web = living(12, "Pipi", Pet::LifeStage::Adult);
    det.reset(web, archive);
    det.markFromWeb(T::Departed); det.markFromWeb(T::MemorialAdded);
    assert(det.detect(web, archive, ev, 8) == 0);  // Nothing changed yet: the mark is dropped.
    assert(web.depart() && archive.append(web));
    det.markFromWeb(T::Departed); det.markFromWeb(T::MemorialAdded);
    n = det.detect(web, archive, ev, 8);
    assert(n == 2 && ev[0].type == T::Departed && ev[0].fromWeb && ev[1].type == T::MemorialAdded && ev[1].fromWeb);
    char eventText[1025]; uint16_t eventLen = 0;
    assert(Ble::makePetEventJson(ev[0], eventText, sizeof(eventText), eventLen) && strstr(eventText, R"("source":"web")"));
    auto next = living(13, "Momo", Pet::LifeStage::Egg);
    det.markFromWeb(T::Adopted);
    n = det.detect(next, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::Adopted && ev[0].petId == 13 && ev[0].fromWeb);
    assert(archive.remove(archive.count() - 1));
    n = det.detect(next, archive, ev, 8);
    assert(n == 1 && ev[0].type == T::MemorialDeleted && !ev[0].fromWeb);
    // Output capacity is respected; extras are dropped, not carried over.
    Storage::Memorials many; for (uint64_t i = 1; i <= 5; ++i) assert(many.append(record(100 + i, "M")));
    assert(det.detect(baby, many, ev, 3) == 3 && det.detect(baby, many, ev, 8) == 0);
}

static void admission() {
    // Capacity 8 with two reserved slots: events fit only while at most five are queued.
    assert(Ble::petEventAdmitted(true, true, 0, 8));
    assert(Ble::petEventAdmitted(true, true, 5, 8));
    assert(!Ble::petEventAdmitted(true, true, 6, 8));
    assert(!Ble::petEventAdmitted(true, true, 8, 8));
    assert(!Ble::petEventAdmitted(false, true, 0, 8));
    assert(!Ble::petEventAdmitted(true, false, 0, 8));
}

int main() {
    sadHysteresis(); memorialHash(); statusJson(); deviceInfo(); eventJson(); detection(); admission();
    puts("PASS: pet sync phase 1 (sad hysteresis, memorial hash, status/device_info JSON and size, pet_event JSON, detection, admission).");
}
