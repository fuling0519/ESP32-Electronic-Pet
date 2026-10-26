#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

namespace Ble {

// set_sound (feature sound): same op and two-step reply as care. Binary device preference,
// stored by the same save path as the Volume screen (0 muted, 1 sound).
struct SoundRequest {
    uint32_t commandId = 0;
    bool muted = false;  // The op target: a replay with the other state is op_conflict.
    uint64_t op = 0;
};
// muted and op of a command whose v/id/cmd were already validated: "ok", missing_field (muted
// absent or not a boolean, op absent or not a string) or invalid_value (op format).
const char* parseSoundCommand(JsonVariantConst message, uint32_t id, SoundRequest& out);
// After the op cache: busy unless activity is idle, menu, sleeping or ended. The Volume screen
// (an unsaved choice), games, animations and web play are all other activities. "ok" when accepted.
const char* soundEligibility(const char* activity);

struct SoundOutcome {
    bool done = false;
    const char* code = nullptr;  // failed only: save_failed (static text).
    bool muted = false;          // done: the mode now in effect.
};
SoundOutcome soundDone(bool muted);
SoundOutcome soundFailed(const char* code);
// action "set_sound"; done carries changes {"muted":<bool>}.
bool makeSoundResultJson(uint32_t id, uint64_t op, const SoundOutcome& outcome,
                         char* out, size_t cap, uint16_t& len);

// Saves first and changes the audible mode only after the save succeeded, so a failure leaves
// both the stored and the in-memory mode as they were. Sound needs volume()/setVolume(),
// Settings saveVolume(); level 0 is muted, 1 enabled.
template <typename Sound, typename Settings>
SoundOutcome applySound(Sound& sound, Settings& settings, bool muted) {
    const uint8_t level = muted ? 0 : 1;
    if (!settings.saveVolume(level)) return soundFailed("save_failed");
    sound.setVolume(level);
    return soundDone(muted);
}

}  // namespace Ble
