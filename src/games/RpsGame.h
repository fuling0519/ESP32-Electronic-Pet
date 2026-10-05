#pragma once

#include <stdint.h>

namespace Games {

enum class RpsMove : uint8_t { Scissors, Rock, Paper };
enum class RpsOutcome : uint8_t { Draw, Win, Loss };
enum class RpsPhase : uint8_t { Inactive, Ready, Select, Countdown, Reveal, Summary, Replay };
enum class RpsInput : uint8_t { None, Left, Right, Confirm, Cancel, ToggleReplay };

// No hardware or pet mutation here. Randomness is supplied by the application.
class RpsGame {
public:
    using RandomSource = uint32_t (*)();
    explicit RpsGame(RandomSource random) : random_(random) {}
    void begin();
    void cancel();
    bool update(RpsInput input, uint32_t now);
    static RpsOutcome judge(RpsMove player, RpsMove opponent);
    RpsPhase phase() const { return phase_; }
    RpsMove player() const { return player_; }
    RpsMove opponent() const { return opponent_; }
    uint8_t round() const { return round_; }
    uint8_t wins() const { return wins_; }
    uint8_t losses() const { return losses_; }
    uint8_t countdown() const { return countdown_; }
    bool replaySelected() const { return replaySelected_; }
    RpsOutcome roundOutcome() const { return judge(player_, opponent_); }
    RpsOutcome outcome() const;
    bool rewardPending() const { return rewardPending_; }
    // Returns the mood reward and consumes the completion token.
    uint8_t takeReward();
    // Apply EXP only together with a nonzero, consumed completion reward.
    uint8_t expReward() const;
private:
    void selectRound();
    RandomSource random_;
    RpsPhase phase_ = RpsPhase::Inactive;
    RpsMove player_ = RpsMove::Rock;
    RpsMove opponent_ = RpsMove::Rock;
    uint8_t round_ = 0, wins_ = 0, losses_ = 0, countdown_ = 3;
    uint32_t phaseStartedAt_ = 0;
    bool rewardPending_ = false, replaySelected_ = false;
};
}  // namespace Games
