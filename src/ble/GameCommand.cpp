#include "ble/GameCommand.h"
#include <string.h>

namespace Ble {

bool isGameCommand(const char* cmd) {
    return cmd && (strcmp(cmd, "game_start") == 0 || strcmp(cmd, "game_heartbeat") == 0 ||
                   strcmp(cmd, "game_finish") == 0 || strcmp(cmd, "game_cancel") == 0);
}

bool parseDecimal(const char* text, uint64_t& value) {
    if (!text || !*text || strlen(text) > 20) return false;
    uint64_t result = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        const uint64_t digit = static_cast<uint64_t>(*p - '0');
        if (result > (UINT64_MAX - digit) / 10) return false;
        result = result * 10 + digit;
    }
    value = result;
    return true;
}

namespace {
// Session ids are non-zero 32-bit decimal strings; anything else names no session.
const char* readSession(JsonVariantConst message, uint32_t& session) {
    JsonVariantConst field = message["session"];
    if (!field.is<const char*>()) return "missing_field";
    uint64_t value = 0;
    if (!parseDecimal(field.as<const char*>(), value) || !value || value > UINT32_MAX) return "invalid_session";
    session = static_cast<uint32_t>(value);
    return "ok";
}
}  // namespace

const char* parseGameCommand(const char* cmd, JsonVariantConst message, uint32_t id, GameRequest& out) {
    GameRequest request;
    request.commandId = id;
    if (strcmp(cmd, "game_start") == 0) {
        JsonVariantConst game = message["game"], pet = message["pet_id"];
        if (!game.is<const char*>() || !pet.is<const char*>()) return "missing_field";
        if (!parseDecimal(pet.as<const char*>(), request.petId)) return "invalid_value";
        if (!Games::WebGameSession::parseKind(game.as<const char*>(), request.kind)) return "invalid_value";
        request.type = GameRequest::Type::Start;
    } else {
        const char* code = readSession(message, request.session);
        if (strcmp(code, "ok") != 0) return code;
        if (strcmp(cmd, "game_heartbeat") == 0) request.type = GameRequest::Type::Heartbeat;
        else if (strcmp(cmd, "game_cancel") == 0) request.type = GameRequest::Type::Cancel;
        else if (strcmp(cmd, "game_finish") == 0) {
            JsonVariantConst throws = message["score"]["throws"], catches = message["score"]["catches"];
            if (!throws.is<int32_t>() || !catches.is<int32_t>()) return "missing_field";
            request.score.throws = throws.as<int32_t>();
            request.score.catches = catches.as<int32_t>();
            request.type = GameRequest::Type::Finish;
        } else return "unknown_command";
    }
    out = request;
    return "ok";
}

}  // namespace Ble
