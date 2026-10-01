#include <assert.h>
#include <stdio.h>
#include "games/RpsGame.h"

using namespace Games;
static uint32_t choice = 0, calls = 0;
static uint32_t randomMove() { ++calls; return choice; }

int main() {
    const RpsOutcome expected[3][3] = {
        {RpsOutcome::Draw, RpsOutcome::Loss, RpsOutcome::Win},
        {RpsOutcome::Win, RpsOutcome::Draw, RpsOutcome::Loss},
        {RpsOutcome::Loss, RpsOutcome::Win, RpsOutcome::Draw}
    };
    for (unsigned p = 0; p < 3; ++p)
        for (unsigned o = 0; o < 3; ++o)
            assert(RpsGame::judge(static_cast<RpsMove>(p), static_cast<RpsMove>(o)) == expected[p][o]);

    // Exhaust every opponent sequence with a fixed player: includes all draws,
    // tied scores, and games decided after round two. Starts near millis wrap.
    for (unsigned sequence = 0; sequence < 27; ++sequence) {
        RpsGame game(randomMove);
        uint32_t now = UINT32_MAX - 500;
        unsigned value = sequence, wins = 0, losses = 0;
        game.begin();
        for (unsigned round = 1; round <= 3; ++round) {
            choice = value % 3; value /= 3;
            const unsigned beforeCalls = calls;
            game.update(RpsInput::Confirm, now);
            assert(game.phase() == RpsPhase::Select && game.round() == round);
            assert(calls == beforeCalls + 1);
            game.update(RpsInput::Left, now);
            game.update(RpsInput::Left, now);
            assert(game.player() == RpsMove::Scissors);
            game.update(RpsInput::Right, now);
            assert(game.player() == RpsMove::Rock && calls == beforeCalls + 1);
            game.update(RpsInput::Confirm, now);
            game.update(RpsInput::Right, now + 349);
            assert(game.player() == RpsMove::Rock && game.countdown() == 3);
            game.update(RpsInput::Confirm, now + 350);
            assert(game.phase() == RpsPhase::Countdown && game.countdown() == 2);
            game.update(RpsInput::None, now + 700);
            assert(game.countdown() == 1);
            game.update(RpsInput::Confirm, now + 1050);
            assert(game.phase() == RpsPhase::Reveal);
            if (choice == 0) ++wins;
            if (choice == 2) ++losses;
            assert(game.wins() == wins && game.losses() == losses);
            game.update(RpsInput::Confirm, now + 1849); // Early click is dropped.
            assert(game.phase() == RpsPhase::Reveal);
            assert(game.rewardPending() == (round == 3));
            if (round != 3) assert(game.takeReward() == 0);
            now += 1850;
        }
        assert(game.takeReward() == (wins > losses ? 15 : wins == losses ? 10 : 5));
        assert(game.takeReward() == 0);
        game.update(RpsInput::Confirm, now);
        assert(game.phase() == RpsPhase::Summary);
        game.update(RpsInput::Confirm, now);
        assert(game.phase() == RpsPhase::Replay && !game.replaySelected());
        game.update(RpsInput::ToggleReplay, now);
        game.update(RpsInput::Confirm, now);
        assert(game.phase() == RpsPhase::Select && game.round() == 1);
        assert(game.wins() == 0 && game.losses() == 0 && game.takeReward() == 0);
        game.update(RpsInput::Cancel, now);
        assert(game.phase() == RpsPhase::Inactive && game.takeReward() == 0);
    }
    // Cancel at the reveal deadline before completion cannot earn a reward.
    RpsGame game(randomMove);
    game.begin(); game.update(RpsInput::Confirm, 0);
    game.update(RpsInput::Confirm, 0);
    game.update(RpsInput::Cancel, 1050);
    game.update(RpsInput::None, 99999);
    assert(game.phase() == RpsPhase::Inactive && game.takeReward() == 0);
    puts("PASS: nine pairings, 27 matches, timing/wrap, locked choice, single reward, replay, cancellation.");
}
