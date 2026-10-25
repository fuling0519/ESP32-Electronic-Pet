#pragma once
#include <stdint.h>

namespace Games {

// Web play games (toy, chorus) started over BLE. The browser only reports what was played; this
// class decides whether a result counts, and remembers the last result so a
// resent finish replies again without paying twice. Hardware-free for native tests.
enum class WebGameKind : uint8_t { None, Toy, Chorus };
enum class WebGameEnd : uint8_t { None, DeviceCancel, HeartbeatTimeout, PetState, Disconnected };

// Each game reads its own pair; -1 means the browser did not send that field.
struct WebGameScore { int32_t throws = -1, catches = -1, correct = -1, total = -1; };
struct WebGameReward { uint8_t mood = 0; uint16_t exp = 0; };
struct WebGameResult { uint8_t moodGain = 0; uint16_t expGain = 0; uint8_t levelsGained = 0; };

class WebGameSession {
public:
    static constexpr uint32_t kHeartbeatMs = 5000;
    static constexpr uint32_t kTimeoutMs = 15000;

    static bool parseKind(const char* name, WebGameKind& kind);
    static const char* kindName(WebGameKind kind);
    static uint32_t minDurationMs(WebGameKind kind);
    static const char* endReason(WebGameEnd reason);

    bool active() const { return active_; }
    uint32_t id() const { return id_; }
    WebGameKind kind() const { return kind_; }

    // Starts a new session and returns its non-zero id.
    uint32_t begin(WebGameKind kind, uint32_t now);
    bool heartbeat(uint32_t session, uint32_t now);

    enum class Finish : uint8_t { Ok, Replay, InvalidSession, TooEarly, InvalidScore };
    // Checks a finish without ending the session; on Ok fills the reward to apply.
    Finish check(uint32_t session, const WebGameScore& score, uint32_t now,
                 WebGameReward& reward) const;
    // Records the actually applied result and ends the session.
    void complete(const WebGameResult& result);
    const WebGameResult& lastResult() const { return lastResult_; }

    // Idempotent: true for the active session or one that has already ended.
    bool cancel(uint32_t session);
    // Ends the active session for a device-side reason; returns false if none was active.
    bool end(WebGameEnd reason);
    // Heartbeat timeout: ends and returns true once when the silence is too long.
    bool expired(uint32_t now);

private:
    bool active_ = false;
    uint32_t id_ = 0, nextId_ = 1, startedAt_ = 0, heartbeatAt_ = 0;
    WebGameKind kind_ = WebGameKind::None;
    uint32_t lastEndedId_ = 0;
    bool lastCompleted_ = false;
    WebGameResult lastResult_{};
};

}  // namespace Games
