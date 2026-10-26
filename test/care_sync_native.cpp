// Pet sync phase 2 without NimBLE: care parsing and op format, eligibility order, the op cache
// (replay, conflict, eviction), action_result JSON and deltas, and the transmit queue priorities.
#include <ArduinoJson.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble/CareCommand.h"
#include "ble/OpCache.h"
#include "ble/PetEvents.h"
#include "ble/TxQueue.h"
#include "pet/PetData.h"

using Ble::CareAction;
using Match = Ble::OpCache::Match;

static Pet::PetData living(uint64_t id, Pet::LifeStage stage) {
    Pet::PetData pet; assert(pet.startNewEgg(id, "Tamama", Pet::SpeciesId::Bird));
    auto s = pet.snapshot(); s.lifeStage = stage;
    s.ageSeconds = stage == Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds :
        stage == Pet::LifeStage::Adult ? Pet::PetData::kAdultAgeSeconds : 0;
    assert(pet.restore(s));
    pet.setMood(80); pet.setSatiety(50); pet.setCleanliness(50);
    return pet;
}
static const char* parse(const char* json, Ble::CareRequest& out) {
    StaticJsonDocument<512> d; assert(!deserializeJson(d, json));
    return Ble::parseCareCommand(d.as<JsonVariantConst>(), 21, out);
}
static void result(uint32_t id, uint64_t op, CareAction a, const Ble::CareOutcome& o, char (&json)[1025]) {
    uint16_t len = 0;
    assert(Ble::makeActionResultJson(id, op, a, o, json, sizeof(json), len) && len == strlen(json));
}

static void opFormat() {
    uint64_t op = 0;
    assert(Ble::parseOp("9f2c41d07ab3e815", op) && op == 0x9f2c41d07ab3e815ULL);
    assert(Ble::parseOp("0000000000000000", op) && op == 0);
    assert(Ble::parseOp("ffffffffffffffff", op) && op == UINT64_MAX);
    assert(!Ble::parseOp("9F2C41D07AB3E815", op));   // Uppercase.
    assert(!Ble::parseOp("9f2c41d07ab3e81", op));    // 15 digits.
    assert(!Ble::parseOp("9f2c41d07ab3e8150", op));  // 17 digits.
    assert(!Ble::parseOp("9f2c41d07ab3e81g", op) && !Ble::parseOp("", op) && !Ble::parseOp(nullptr, op));
    assert(!Ble::parseOp(" 9f2c41d07ab3e81", op) && !Ble::parseOp("0x2c41d07ab3e815", op));
    Ble::CareRequest r;
    assert(strcmp(parse(R"({"action":"feed","pet_id":"42","op":"9f2c41d07ab3e815"})", r), "ok") == 0);
    assert(r.commandId == 21 && r.action == CareAction::Feed && r.petId == 42 && r.op == 0x9f2c41d07ab3e815ULL);
    const char* names[] = {"feed", "clean", "treat", "sleep", "wake"};
    for (uint8_t i = 0; i < 5; ++i) {
        char json[128];
        snprintf(json, sizeof(json), R"({"action":"%s","pet_id":"7","op":"00000000000000a%u"})", names[i], i);
        assert(strcmp(parse(json, r), "ok") == 0 && static_cast<uint8_t>(r.action) == i && r.op == 0xa0u + i);
        assert(strcmp(Ble::careActionName(r.action), names[i]) == 0);
    }
    // Missing or wrongly typed fields, then format errors.
    assert(strcmp(parse(R"({"action":"feed","pet_id":"42"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"action":"feed","pet_id":"42","op":12})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"pet_id":"42","op":"9f2c41d07ab3e815"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"action":"feed","pet_id":42,"op":"9f2c41d07ab3e815"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"action":"feed","pet_id":"42","op":"9F2C41D07AB3E815"})", r), "invalid_value") == 0);
    assert(strcmp(parse(R"({"action":"feed","pet_id":"42","op":"9f2c"})", r), "invalid_value") == 0);
    assert(strcmp(parse(R"({"action":"play","pet_id":"42","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
    assert(strcmp(parse(R"({"action":"Feed","pet_id":"42","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
    assert(strcmp(parse(R"({"action":"feed","pet_id":"-4","op":"9f2c41d07ab3e815"})", r), "invalid_value") == 0);
}

static void eligibility() {
    using Ble::careEligibility;
    auto pet = living(42, Pet::LifeStage::Adult);
    const CareAction awake[] = {CareAction::Feed, CareAction::Clean, CareAction::Sleep};
    for (CareAction a : awake) {
        assert(strcmp(careEligibility(a, 42, pet, "idle"), "ok") == 0);
        assert(strcmp(careEligibility(a, 42, pet, "menu"), "ok") == 0);
        const char* busy[] = {"sleeping", "ended", "device_game", "web_game", "busy", "future_value"};
        for (const char* activity : busy) assert(strcmp(careEligibility(a, 42, pet, activity), "busy") == 0);
        // Order: pet_changed before not_allowed before busy.
        assert(strcmp(careEligibility(a, 43, pet, "busy"), "pet_changed") == 0);
    }
    // Treat needs sickness; feeding a sick pet is allowed.
    assert(strcmp(careEligibility(CareAction::Treat, 42, pet, "idle"), "not_allowed") == 0);
    assert(strcmp(careEligibility(CareAction::Treat, 42, pet, "busy"), "not_allowed") == 0);
    pet.setSick(true);
    assert(strcmp(careEligibility(CareAction::Treat, 42, pet, "menu"), "ok") == 0);
    assert(strcmp(careEligibility(CareAction::Feed, 42, pet, "idle"), "ok") == 0);
    assert(strcmp(careEligibility(CareAction::Treat, 42, pet, "web_game"), "busy") == 0);
    pet.setSick(false);
    // Wake needs normal sleep and the sleeping activity; sleeping pets cannot be fed.
    assert(strcmp(careEligibility(CareAction::Wake, 42, pet, "sleeping"), "not_allowed") == 0);
    assert(pet.beginNormalSleep());
    assert(strcmp(careEligibility(CareAction::Wake, 42, pet, "sleeping"), "ok") == 0);
    assert(strcmp(careEligibility(CareAction::Wake, 42, pet, "idle"), "busy") == 0);
    assert(strcmp(careEligibility(CareAction::Wake, 42, pet, "menu"), "busy") == 0);
    assert(strcmp(careEligibility(CareAction::Feed, 42, pet, "sleeping"), "not_allowed") == 0);
    assert(strcmp(careEligibility(CareAction::Sleep, 42, pet, "sleeping"), "not_allowed") == 0);
    assert(strcmp(careEligibility(CareAction::Wake, 9, pet, "sleeping"), "pet_changed") == 0);
    // Eggs and ended pets.
    auto egg = living(5, Pet::LifeStage::Egg);
    for (uint8_t i = 0; i < 5; ++i)
        assert(strcmp(careEligibility(static_cast<CareAction>(i), 5, egg, "menu"), "not_allowed") == 0);
    assert(strcmp(careEligibility(CareAction::Wake, 5, egg, "sleeping"), "not_allowed") == 0);
    auto gone = living(6, Pet::LifeStage::Baby); assert(gone.depart());
    for (uint8_t i = 0; i < 5; ++i)
        assert(strcmp(careEligibility(static_cast<CareAction>(i), 6, gone, "idle"), "not_allowed") == 0);
    auto dead = living(8, Pet::LifeStage::Adult); dead.setSick(true); dead.setDead(true);
    assert(strcmp(careEligibility(CareAction::Treat, 8, dead, "idle"), "not_allowed") == 0);
}

static void opCache() {
    Ble::OpCache cache;
    const uint8_t feed = static_cast<uint8_t>(CareAction::Feed), clean = static_cast<uint8_t>(CareAction::Clean);
    assert(cache.check(1, Ble::OpKind::Care, feed, 42, 10) == Match::New && !cache.find(1));
    cache.begin(1, Ble::OpKind::Care, feed, 42, 10);
    // Pending: ok again, and the latest command id answers.
    assert(cache.check(1, Ble::OpKind::Care, feed, 42, 11) == Match::Pending && cache.find(1)->commandId == 11);
    // Another action or target for the same op is a conflict and keeps the entry.
    assert(cache.check(1, Ble::OpKind::Care, clean, 42, 12) == Match::Conflict);
    assert(cache.check(1, Ble::OpKind::Care, feed, 43, 12) == Match::Conflict && cache.find(1)->commandId == 11);
    Ble::CareOutcome o; o.done = true; o.changes.satiety = 20; o.changes.exp = 3;
    const Ble::OpCache::Entry* e = cache.complete(1, o);
    assert(e && e->done && e->commandId == 11 && e->outcome.changes.satiety == 20);
    assert(cache.check(1, Ble::OpKind::Care, feed, 42, 13) == Match::Done && cache.find(1)->commandId == 13);
    // The replayed action_result is identical apart from the new id.
    char first[1025], again[1025];
    result(11, 1, CareAction::Feed, o, first); result(13, 1, CareAction::Feed, cache.find(1)->outcome, again);
    assert(strstr(first, "\"id\":11,") && strstr(again, "\"id\":13,"));
    assert(strcmp(strstr(first, "\"op\""), strstr(again, "\"op\"")) == 0);
    // Eviction: 8 entries; the oldest finished entry goes first, a pending op keeps its slot.
    Ble::OpCache full;
    full.begin(100, Ble::OpKind::Care, feed, 1, 1);  // Stays pending.
    for (uint64_t op = 101; op < 108; ++op) { full.begin(op, Ble::OpKind::Care, feed, 1, 1); full.complete(op, o); }
    for (uint64_t op = 100; op < 108; ++op) assert(full.find(op));
    full.begin(108, Ble::OpKind::Care, feed, 1, 1);
    assert(full.find(100) && !full.find(101) && full.find(102) && full.find(108));
    full.complete(108, o);
    full.begin(109, Ble::OpKind::Care, feed, 1, 1);
    assert(!full.find(102) && full.find(103) && full.find(100));
    assert(full.check(101, Ble::OpKind::Care, clean, 1, 2) == Match::New);  // Forgotten: runs as new.
    assert(!full.complete(101, o));
    // Every slot pending (cannot happen while care is busy, but stays bounded): the oldest goes.
    Ble::OpCache pending;
    for (uint64_t op = 1; op <= 9; ++op) pending.begin(op, Ble::OpKind::Care, feed, 1, 1);
    assert(!pending.find(1) && pending.find(2) && pending.find(9));
}

static void actionResults() {
    char json[1025];
    // Feed: actual satiety and EXP, levels gained; the example from the spec.
    auto pet = living(42, Pet::LifeStage::Adult); auto before = pet;
    assert(pet.feed());
    Ble::CareOutcome o = Ble::careDone(before, pet);
    result(21, 0x9f2c41d07ab3e815ULL, CareAction::Feed, o, json);
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":21,"op":"9f2c41d07ab3e815","action":"feed","outcome":"done","changes":{"satiety":20,"exp":3,"levels_gained":0}})") == 0);
    // Full values: done with zero deltas and no EXP.
    pet.setSatiety(100); before = pet; assert(pet.feed());
    result(22, 1, CareAction::Feed, Ble::careDone(before, pet), json);
    assert(strstr(json, R"("outcome":"done","changes":{"satiety":0,"exp":0,"levels_gained":0}})"));
    pet.setSatiety(95); before = pet; assert(pet.feed());
    result(22, 1, CareAction::Feed, Ble::careDone(before, pet), json);
    assert(strstr(json, R"("changes":{"satiety":5,"exp":3,"levels_gained":0}})"));
    // Clean with a level up: EXP counts across the level boundary.
    pet.setCleanliness(10); pet.setLevel(1); pet.setExp(48); before = pet; assert(pet.clean());
    o = Ble::careDone(before, pet);
    assert(pet.level() == 2 && pet.exp() == 1 && o.changes.exp == 3 && o.changes.levelsGained == 1);
    result(23, 2, CareAction::Clean, o, json);
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":23,"op":"0000000000000002","action":"clean","outcome":"done","changes":{"cleanliness":30,"exp":3,"levels_gained":1}})") == 0);
    pet.setCleanliness(100); before = pet; assert(pet.clean());
    result(24, 3, CareAction::Clean, Ble::careDone(before, pet), json);
    assert(strstr(json, R"("changes":{"cleanliness":0,"exp":0,"levels_gained":0}})"));
    // Max level: no EXP is accepted.
    pet.setLevel(Pet::PetData::kMaxLevel); pet.setSatiety(0); before = pet; assert(pet.feed());
    o = Ble::careDone(before, pet); assert(o.changes.satiety == 20 && o.changes.exp == 0 && !o.changes.levelsGained);
    // Treat: EXP, levels and growth held back by the sickness.
    auto baby = living(42, Pet::LifeStage::Baby);
    auto s = baby.snapshot(); s.ageSeconds = Pet::PetData::kAdultAgeSeconds; assert(baby.restore(s));
    baby.setSick(true); before = baby; assert(baby.treat());
    o = Ble::careDone(before, baby);
    result(25, 4, CareAction::Treat, o, json);
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":25,"op":"0000000000000004","action":"treat","outcome":"done","changes":{"exp":5,"levels_gained":0,"grew":true}})") == 0);
    auto adult = living(42, Pet::LifeStage::Adult); adult.setSick(true); before = adult; assert(adult.treat());
    result(26, 5, CareAction::Treat, Ble::careDone(before, adult), json);
    assert(strstr(json, R"("changes":{"exp":5,"levels_gained":0,"grew":false}})"));
    // Sleep: empty changes. Wake: growth after sleeping past the adult age.
    before = adult; assert(adult.beginNormalSleep());
    result(27, 6, CareAction::Sleep, Ble::careDone(before, adult), json);
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":27,"op":"0000000000000006","action":"sleep","outcome":"done","changes":{}})") == 0);
    before = adult; assert(adult.wake());
    result(28, 7, CareAction::Wake, Ble::careDone(before, adult), json);
    assert(strstr(json, R"("action":"wake","outcome":"done","changes":{"grew":false}})"));
    auto sleeper = living(42, Pet::LifeStage::Baby); assert(sleeper.beginNormalSleep());
    s = sleeper.snapshot(); s.ageSeconds = Pet::PetData::kAdultAgeSeconds; assert(sleeper.restore(s));
    assert(sleeper.lifeStage() == Pet::LifeStage::Baby);
    before = sleeper; assert(sleeper.wake() && sleeper.lifeStage() == Pet::LifeStage::Adult);
    result(29, 8, CareAction::Wake, Ble::careDone(before, sleeper), json);
    assert(strstr(json, R"("changes":{"grew":true}})"));
    // Failed: a code and no changes.
    const char* codes[] = {"not_allowed", "pet_changed", "save_failed"};
    for (const char* code : codes) {
        result(30, 9, CareAction::Sleep, Ble::careFailed(code), json);
        char expected[200];
        snprintf(expected, sizeof(expected), R"({"v":1,"type":"action_result","id":30,"op":"0000000000000009","action":"sleep","outcome":"failed","code":"%s"})", code);
        assert(strcmp(json, expected) == 0);
    }
    // Longest message stays small (spec estimate 173 bytes).
    o = Ble::CareOutcome{}; o.done = true; o.changes.satiety = 100; o.changes.exp = 65535; o.changes.levelsGained = 19;
    uint16_t len = 0;
    assert(Ble::makeActionResultJson(UINT32_MAX, UINT64_MAX, CareAction::Feed, o, json, sizeof(json), len) && len < 173);
    // A treatment interrupted before it settled.
    Ble::PendingTreatment pending; Ble::CareRequest r; r.petId = 42; r.op = 77; r.action = CareAction::Treat;
    Ble::CareOutcome out;
    assert(!pending.interrupted(false, adult, out));  // Nothing pending.
    pending.start(r);
    assert(!pending.interrupted(true, adult, out) && pending.active());
    assert(pending.interrupted(false, adult, out) && !pending.active() && !out.done && strcmp(out.code, "not_allowed") == 0);
    auto other = living(43, Pet::LifeStage::Adult);
    pending.start(r); assert(pending.interrupted(false, other, out) && strcmp(out.code, "pet_changed") == 0);
}

static void txQueue() {
    Ble::TxQueue q; Ble::TxQueue::Message m;
    bool dropped = false;
    const auto push = [&](const char* json, Ble::TxKind kind) {
        return q.push(json, static_cast<uint16_t>(strlen(json)), kind, &dropped);
    };
    // Status replaces a queued status in place.
    assert(push("s1", Ble::TxKind::Status) && push("r1", Ble::TxKind::Reply) && push("s2", Ble::TxKind::Status));
    assert(q.count() == 2 && q.pop(m) && strcmp(m.json, "s2") == 0 && m.kind == Ble::TxKind::Status);
    assert(q.pop(m) && strcmp(m.json, "r1") == 0 && !q.pop(m));
    // Events leave two slots; replies then fill the queue.
    for (int i = 0; i < 8; ++i) push("e", Ble::TxKind::Event);
    assert(q.count() == 8 - Ble::kEventReservedSlots);
    assert(push("s", Ble::TxKind::Status) && q.count() == 7);
    assert(push("r1", Ble::TxKind::Reply) && q.count() == 8 && !dropped);
    // Full: each reply displaces the newest event, then the status (reported), never a reply.
    for (int i = 2; i <= 7; ++i) { char json[4]; snprintf(json, sizeof(json), "r%d", i); assert(push(json, Ble::TxKind::Reply) && !dropped); }
    assert(q.count() == 8 && !push("e", Ble::TxKind::Event));
    // The queued status is replaced, not added, while it is still waiting.
    assert(push("s9", Ble::TxKind::Status) && q.count() == 8);
    assert(push("r8", Ble::TxKind::Reply) && dropped && q.count() == 8);
    for (uint8_t i = 0; i < 8; ++i) assert(q.kindAt(i) == Ble::TxKind::Reply);
    assert(!push("r9", Ble::TxKind::Reply) && q.count() == 8);  // Only when 8 replies wait.
    for (int i = 1; i <= 8; ++i) {
        char json[4]; snprintf(json, sizeof(json), "r%d", i);
        assert(q.pop(m) && strcmp(m.json, json) == 0 && m.length == strlen(json));
    }
    assert(!q.pop(m) && q.count() == 0);
    // Oversized or empty messages are refused.
    static char big[Ble::kMaxMessageBytes + 2]; memset(big, 'x', sizeof(big) - 1); big[sizeof(big) - 1] = '\0';
    assert(!q.push(big, Ble::kMaxMessageBytes + 1, Ble::TxKind::Reply) && !q.push("", 0, Ble::TxKind::Reply));
    big[Ble::kMaxMessageBytes] = '\0';
    assert(q.push(big, Ble::kMaxMessageBytes, Ble::TxKind::Reply) && q.pop(m) && m.length == Ble::kMaxMessageBytes);
    q.push("a", 1, Ble::TxKind::Reply); q.clear(); assert(!q.count());
}

int main() {
    opFormat();
    eligibility();
    opCache();
    actionResults();
    txQueue();
    puts("PASS: care op format, eligibility order, op replay/conflict/eviction, action_result JSON and transmit priorities.");
}
