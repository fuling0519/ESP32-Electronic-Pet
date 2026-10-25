#include "games/WebGameSession.h"
#include <string.h>

namespace Games {

bool WebGameSession::parseKind(const char* name, WebGameKind& kind) {
    if (name && strcmp(name, "toy") == 0) { kind = WebGameKind::Toy; return true; }
    if (name && strcmp(name, "chorus") == 0) { kind = WebGameKind::Chorus; return true; }
    return false;
}

const char* WebGameSession::kindName(WebGameKind kind) {
    return kind == WebGameKind::Toy ? "toy" : kind == WebGameKind::Chorus ? "chorus" : "";
}

uint32_t WebGameSession::minDurationMs(WebGameKind kind) {
    // Chorus: three sung tunes plus answers and feedback take well over 12 s even when played quickly.
    return kind == WebGameKind::Toy ? 15000 : kind == WebGameKind::Chorus ? 12000 : 0;
}

const char* WebGameSession::endReason(WebGameEnd reason) {
    switch (reason) {
        case WebGameEnd::DeviceCancel: return "device_cancel";
        case WebGameEnd::HeartbeatTimeout: return "heartbeat_timeout";
        case WebGameEnd::PetState: return "pet_state";
        case WebGameEnd::Disconnected: return "disconnected";
        case WebGameEnd::None: break;
    }
    return "";
}

uint32_t WebGameSession::begin(WebGameKind kind, uint32_t now) {
    id_ = nextId_++;
    if (!nextId_) nextId_ = 1;
    active_ = true; kind_ = kind; startedAt_ = heartbeatAt_ = now;
    return id_;
}

bool WebGameSession::heartbeat(uint32_t session, uint32_t now) {
    if (!active_ || session != id_) return false;
    heartbeatAt_ = now;
    return true;
}

WebGameSession::Finish WebGameSession::check(uint32_t session, const WebGameScore& score,
                                             uint32_t now, WebGameReward& reward) const {
    if (!active_) return session && session == lastEndedId_ && lastCompleted_ ?
        Finish::Replay : Finish::InvalidSession;
    if (session != id_) return Finish::InvalidSession;
    if (kind_ == WebGameKind::Toy &&
        (score.throws != 5 || score.catches < 0 || score.catches > 5)) return Finish::InvalidScore;
    if (kind_ == WebGameKind::Chorus &&
        (score.total != 3 || score.correct < 0 || score.correct > 3)) return Finish::InvalidScore;
    if (now - startedAt_ < minDurationMs(kind_)) return Finish::TooEarly;
    if (kind_ == WebGameKind::Chorus) {
        // Same tiers as the web memory notes: by tunes sung right out of three.
        reward.mood = score.correct == 3 ? 10 : score.correct == 2 ? 6 : 3;
        reward.exp = score.correct == 3 ? 15 : score.correct == 2 ? 10 : 5;
    } else {
        // Toy play is not a contest: one fixed reward for a completed session.
        reward.mood = 6; reward.exp = 10;
    }
    return Finish::Ok;
}

void WebGameSession::complete(const WebGameResult& result) {
    lastResult_ = result;
    lastEndedId_ = id_; lastCompleted_ = true;
    active_ = false; kind_ = WebGameKind::None;
}

bool WebGameSession::cancel(uint32_t session) {
    if (active_ && session == id_) {
        lastEndedId_ = id_; lastCompleted_ = false;
        active_ = false; kind_ = WebGameKind::None;
        return true;
    }
    return session && session == lastEndedId_;
}

bool WebGameSession::end(WebGameEnd reason) {
    if (!active_ || reason == WebGameEnd::None) return false;
    lastEndedId_ = id_; lastCompleted_ = false;
    active_ = false; kind_ = WebGameKind::None;
    return true;
}

bool WebGameSession::expired(uint32_t now) {
    if (!active_ || now - heartbeatAt_ <= kTimeoutMs) return false;
    return end(WebGameEnd::HeartbeatTimeout);
}

}  // namespace Games
