// Web play session rules and BLE game command parsing, without NimBLE or hardware.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble/GameCommand.h"
#include "games/WebGameSession.h"

using Games::WebGameSession;
using Games::WebGameKind;
using Games::WebGameEnd;
using Finish = WebGameSession::Finish;

static Games::WebGameScore score(int throws, int catches) { Games::WebGameScore s; s.throws = throws; s.catches = catches; return s; }

static const char* parse(const char* json, Ble::GameRequest& out) {
    StaticJsonDocument<384> d;
    assert(!deserializeJson(d, json));
    return Ble::parseGameCommand(d["cmd"].as<const char*>(), d.as<JsonVariantConst>(), 9, out);
}

static void sessionRules() {
    WebGameSession s;
    WebGameKind kind;
    assert(WebGameSession::parseKind("toy", kind) && kind == WebGameKind::Toy);
    assert(WebGameSession::parseKind("chorus", kind) && kind == WebGameKind::Chorus);
    assert(!WebGameSession::parseKind("rps", kind) && !WebGameSession::parseKind(nullptr, kind));
    assert(WebGameSession::minDurationMs(WebGameKind::Chorus) == 12000 && strcmp(WebGameSession::kindName(WebGameKind::Chorus), "chorus") == 0);
    assert(WebGameSession::minDurationMs(WebGameKind::Toy) == 15000);
    const uint32_t id = s.begin(WebGameKind::Toy, 1000);
    assert(id != 0 && s.active() && s.id() == id);
    Games::WebGameReward reward;
    // Too early, wrong session, impossible scores: the session stays open.
    assert(s.check(id, score(5, 3), 15999, reward) == Finish::TooEarly);
    assert(s.check(id + 1, score(5, 3), 20000, reward) == Finish::InvalidSession);
    assert(s.check(id, score(4, 3), 20000, reward) == Finish::InvalidScore);
    assert(s.check(id, score(5, 6), 20000, reward) == Finish::InvalidScore);
    assert(s.check(id, score(5, -1), 20000, reward) == Finish::InvalidScore);
    assert(s.active());
    assert(s.check(id, score(5, 0), 16000, reward) == Finish::Ok);
    assert(reward.mood == 6 && reward.exp == 10);
    Games::WebGameResult applied; applied.moodGain = 4; applied.expGain = 10; applied.levelsGained = 1;
    s.complete(applied);
    assert(!s.active());
    // A resent finish replays the recorded result instead of paying again.
    assert(s.check(id, score(5, 0), 30000, reward) == Finish::Replay);
    assert(s.lastResult().moodGain == 4 && s.lastResult().levelsGained == 1);
    assert(s.check(id + 7, score(5, 0), 30000, reward) == Finish::InvalidSession);
    assert(s.cancel(id) && !s.active());  // cancelling an ended session is idempotent

    // Heartbeats keep it alive; 15 s of silence ends it once.
    const uint32_t second = s.begin(WebGameKind::Toy, 50000);
    assert(second == id + 1);
    assert(!s.heartbeat(id, 54000) && s.heartbeat(second, 54000));
    assert(!s.expired(69000) && s.expired(69001) && !s.active() && !s.expired(90000));
    assert(s.check(second, score(5, 5), 90000, reward) == Finish::InvalidSession);  // timed out: no reward

    // Device-side ends and cancel.
    const uint32_t third = s.begin(WebGameKind::Toy, 100000);
    assert(!s.end(WebGameEnd::None) && s.active());
    assert(s.end(WebGameEnd::DeviceCancel) && !s.active() && !s.end(WebGameEnd::PetState));
    const uint32_t fourth = s.begin(WebGameKind::Toy, 200000);
    assert(fourth != third && !s.cancel(third + 100) && s.active());
    assert(s.cancel(fourth) && !s.active() && s.cancel(fourth));
    assert(s.check(fourth, score(5, 2), 300000, reward) == Finish::InvalidSession);  // cancelled: no replay
    assert(strcmp(WebGameSession::endReason(WebGameEnd::HeartbeatTimeout), "heartbeat_timeout") == 0);
    // millis() rollover keeps durations right.
    WebGameSession r;
    const uint32_t rolled = r.begin(WebGameKind::Toy, 0xFFFFF000u);
    assert(r.check(rolled, score(5, 1), 0xFFFFF000u + 15000u, reward) == Finish::Ok);

    // Chorus: correct/total out of three tunes, tiered like the web memory notes; a toy score does not count.
    WebGameSession c;
    const uint32_t sung = c.begin(WebGameKind::Chorus, 0);
    Games::WebGameScore chorus; chorus.total = 3; chorus.correct = 3;
    assert(c.check(sung, chorus, 11999, reward) == Finish::TooEarly);
    assert(c.check(sung, score(5, 3), 12000, reward) == Finish::InvalidScore);
    Games::WebGameScore bad = chorus; bad.total = 4; assert(c.check(sung, bad, 12000, reward) == Finish::InvalidScore);
    bad = chorus; bad.correct = 4; assert(c.check(sung, bad, 12000, reward) == Finish::InvalidScore);
    bad = chorus; bad.correct = -1; assert(c.check(sung, bad, 12000, reward) == Finish::InvalidScore);
    const int tiers[4][2] = {{3, 5}, {3, 5}, {6, 10}, {10, 15}};
    for (int right = 0; right <= 3; ++right) {
        chorus.correct = right;
        assert(c.check(sung, chorus, 12000, reward) == Finish::Ok);
        assert(reward.mood == tiers[right][0] && reward.exp == tiers[right][1]);
    }
    // A toy session still ignores chorus fields.
    WebGameSession t;
    const uint32_t toy = t.begin(WebGameKind::Toy, 0);
    assert(t.check(toy, chorus, 20000, reward) == Finish::InvalidScore);
}

static void commandParsing() {
    Ble::GameRequest r;
    assert(Ble::isGameCommand("game_start") && Ble::isGameCommand("game_cancel") && !Ble::isGameCommand("get_status"));
    assert(strcmp(parse("{\"cmd\":\"game_start\",\"game\":\"toy\",\"pet_id\":\"18446744073709551615\"}", r), "ok") == 0);
    assert(r.type == Ble::GameRequest::Type::Start && r.kind == WebGameKind::Toy && r.petId == UINT64_MAX && r.commandId == 9);
    assert(strcmp(parse("{\"cmd\":\"game_start\",\"game\":\"chorus\",\"pet_id\":\"42\"}", r), "ok") == 0 && r.kind == WebGameKind::Chorus);
    assert(strcmp(parse("{\"cmd\":\"game_start\",\"game\":\"rps\",\"pet_id\":\"42\"}", r), "invalid_value") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_start\",\"game\":\"toy\",\"pet_id\":42}", r), "missing_field") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_start\",\"game\":\"toy\",\"pet_id\":\"-4\"}", r), "invalid_value") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_start\",\"pet_id\":\"42\"}", r), "missing_field") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_heartbeat\",\"session\":\"3051\"}", r), "ok") == 0);
    assert(r.type == Ble::GameRequest::Type::Heartbeat && r.session == 3051);
    assert(strcmp(parse("{\"cmd\":\"game_cancel\",\"session\":\"0\"}", r), "invalid_session") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_cancel\",\"session\":\"4294967296\"}", r), "invalid_session") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_cancel\",\"session\":3051}", r), "missing_field") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_finish\",\"session\":\"7\",\"score\":{\"throws\":5,\"catches\":2}}", r), "ok") == 0);
    assert(r.type == Ble::GameRequest::Type::Finish && r.score.throws == 5 && r.score.catches == 2);
    // A partial pair parses; the session rejects it for its game (catches stays -1 here).
    assert(strcmp(parse("{\"cmd\":\"game_finish\",\"session\":\"7\",\"score\":{\"throws\":5}}", r), "ok") == 0 && r.score.catches == -1);
    assert(strcmp(parse("{\"cmd\":\"game_finish\",\"session\":\"7\",\"score\":{\"correct\":2,\"total\":3}}", r), "ok") == 0 && r.score.correct == 2 && r.score.total == 3 && r.score.throws == -1);
    assert(strcmp(parse("{\"cmd\":\"game_finish\",\"session\":\"7\",\"score\":{\"correct\":\"2\",\"total\":3}}", r), "missing_field") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_finish\",\"session\":\"7\",\"score\":{}}", r), "missing_field") == 0);
    assert(strcmp(parse("{\"cmd\":\"game_finish\",\"session\":\"7\"}", r), "missing_field") == 0);
    uint64_t v = 0;
    assert(!Ble::parseDecimal("", v) && !Ble::parseDecimal(" 1", v) && !Ble::parseDecimal("18446744073709551616", v));
    assert(Ble::parseDecimal("0", v) && v == 0);
}

int main() {
    sessionRules();
    commandParsing();
    puts("PASS: web play session start/heartbeat/timeout, finish checks, single payout with replay, cancel/end idempotence, rollover and BLE game command parsing, including chorus scores and tiers.");
}
