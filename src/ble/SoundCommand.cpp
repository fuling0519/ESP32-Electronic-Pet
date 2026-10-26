#include "ble/SoundCommand.h"
#include <stdio.h>
#include <string.h>
#include "ble/CareCommand.h"
#include "ble/Messages.h"

namespace Ble {

const char* parseSoundCommand(JsonVariantConst message, uint32_t id, SoundRequest& out) {
    JsonVariantConst muted = message["muted"], op = message["op"];
    if (!muted.is<bool>() || !op.is<const char*>()) return "missing_field";
    SoundRequest request;
    request.commandId = id; request.muted = muted.as<bool>();
    if (!parseOp(op.as<const char*>(), request.op)) return "invalid_value";
    out = request;
    return "ok";
}

const char* soundEligibility(const char* activity) {
    if (!activity) return "busy";
    const bool allowed = strcmp(activity, "idle") == 0 || strcmp(activity, "menu") == 0 ||
        strcmp(activity, "sleeping") == 0 || strcmp(activity, "ended") == 0;
    return allowed ? "ok" : "busy";
}

SoundOutcome soundDone(bool muted) {
    SoundOutcome o; o.done = true; o.muted = muted;
    return o;
}

SoundOutcome soundFailed(const char* code) {
    SoundOutcome o; o.code = code;
    return o;
}

bool makeSoundResultJson(uint32_t id, uint64_t op, const SoundOutcome& o, char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<JSON_OBJECT_SIZE(8) + JSON_OBJECT_SIZE(3) + 32> d;
    char opText[17];
    snprintf(opText, sizeof(opText), "%016llx", static_cast<unsigned long long>(op));
    d["v"] = 1; d["type"] = "action_result"; d["id"] = id; d["op"] = opText;
    d["action"] = "set_sound";
    if (!o.done) {
        d["outcome"] = "failed"; d["code"] = o.code ? o.code : "save_failed";
    } else {
        d["outcome"] = "done";
        d.createNestedObject("changes")["muted"] = o.muted;
    }
    if (d.overflowed()) return false;
    const size_t written = serializeJson(d, out, cap);
    if (!written || written >= cap || written > kMaxMessageBytes) return false;
    len = static_cast<uint16_t>(written);
    return true;
}

}  // namespace Ble
