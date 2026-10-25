#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include "games/WebGameSession.h"

namespace Ble {

// One Web play command, parsed and type-checked; the application decides the outcome.
struct GameRequest {
    enum class Type : uint8_t { None, Start, Heartbeat, Finish, Cancel };
    Type type = Type::None;
    uint32_t commandId = 0;
    Games::WebGameKind kind = Games::WebGameKind::None;
    uint64_t petId = 0;
    uint32_t session = 0;
    Games::WebGameScore score{};
};

bool isGameCommand(const char* cmd);
// Parses game fields for a command whose v/id/cmd were already validated.
// Returns "ok" or a protocol error code (missing_field, invalid_value, invalid_session).
const char* parseGameCommand(const char* cmd, JsonVariantConst message, uint32_t id, GameRequest& out);
// Decimal strings only (no sign, spaces or empty); rejects values that overflow.
bool parseDecimal(const char* text, uint64_t& value);

}  // namespace Ble
