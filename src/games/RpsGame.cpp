#include "games/RpsGame.h"

namespace Games {
void RpsGame::begin() {
    phase_ = RpsPhase::Ready;
    player_ = RpsMove::Rock;
    round_ = wins_ = losses_ = 0;
    countdown_ = 3;
    rewardPending_ = replaySelected_ = false;
}

void RpsGame::cancel() {
    phase_ = RpsPhase::Inactive;
    rewardPending_ = false;
}

void RpsGame::selectRound() {
    // UINT32_MAX is the one excess value: reject it to avoid modulo bias.
    uint32_t value;
    do { value = random_(); } while (value == UINT32_MAX);
    opponent_ = static_cast<RpsMove>(value % 3);
    ++round_;
    phase_ = RpsPhase::Select;
}

RpsOutcome RpsGame::judge(RpsMove player, RpsMove opponent) {
    if (player == opponent) return RpsOutcome::Draw;
    return (static_cast<unsigned>(player) + 3 - static_cast<unsigned>(opponent)) % 3 == 1
        ? RpsOutcome::Win : RpsOutcome::Loss;
}

RpsOutcome RpsGame::outcome() const {
    return wins_ == losses_ ? RpsOutcome::Draw :
        wins_ > losses_ ? RpsOutcome::Win : RpsOutcome::Loss;
}

uint8_t RpsGame::takeReward() {
    if (!rewardPending_) return 0;
    rewardPending_ = false;
    return outcome() == RpsOutcome::Win ? 10 : outcome() == RpsOutcome::Draw ? 6 : 3;
}

uint8_t RpsGame::expReward() const {
    return outcome() == RpsOutcome::Win ? 15 : outcome() == RpsOutcome::Draw ? 10 : 5;
}

bool RpsGame::update(RpsInput input, uint32_t now) {
    if (phase_ == RpsPhase::Inactive) return false;
    if (input == RpsInput::Cancel) { cancel(); return true; }
    switch (phase_) {
        case RpsPhase::Ready:
            if (input == RpsInput::Confirm) { selectRound(); return true; }
            break;
        case RpsPhase::Select:
            if (input == RpsInput::Left && player_ != RpsMove::Scissors) {
                player_ = static_cast<RpsMove>(static_cast<unsigned>(player_) - 1);
                return true;
            }
            if (input == RpsInput::Right && player_ != RpsMove::Paper) {
                player_ = static_cast<RpsMove>(static_cast<unsigned>(player_) + 1);
                return true;
            }
            if (input == RpsInput::Confirm) {
                phaseStartedAt_ = now;
                countdown_ = 3;
                phase_ = RpsPhase::Countdown;
                return true;
            }
            break;
        case RpsPhase::Countdown: {
            const uint32_t elapsed = now - phaseStartedAt_;
            if (elapsed >= 1050) {
                const RpsOutcome result = roundOutcome();
                if (result == RpsOutcome::Win) ++wins_;
                if (result == RpsOutcome::Loss) ++losses_;
                rewardPending_ = round_ == 3;
                phase_ = RpsPhase::Reveal;
                phaseStartedAt_ = now;  // Full reading time even after a late update.
                return true;  // Do not reuse this event on the reveal page.
            }
            const uint8_t digit = 3 - elapsed / 350;
            if (digit != countdown_) { countdown_ = digit; return true; }
            break;
        }
        case RpsPhase::Reveal:
            if (input == RpsInput::Confirm && now - phaseStartedAt_ >= 800) {
                if (round_ == 3) phase_ = RpsPhase::Summary;
                else selectRound();
                return true;
            }
            break;
        case RpsPhase::Summary:
            if (input == RpsInput::Confirm) {
                replaySelected_ = false;
                phase_ = RpsPhase::Replay;
                return true;
            }
            break;
        case RpsPhase::Replay:
            if (input == RpsInput::ToggleReplay) {
                replaySelected_ = !replaySelected_;
                return true;
            }
            if (input == RpsInput::Confirm) {
                if (replaySelected_) { begin(); selectRound(); }
                else cancel();
                return true;
            }
            break;
        case RpsPhase::Inactive: break;
    }
    return false;
}
}  // namespace Games
