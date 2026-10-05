#include <assert.h>
#include <stdio.h>
#include "games/MemoryNotes.h"
using namespace Games;
static uint32_t value=0, calls=0;
static uint32_t randomNote() { ++calls; return value; }
static void playback(MemoryNotes& game,uint32_t& now) {
    for (unsigned i=0;i<game.length();++i) {
        assert(game.activeDirection()==static_cast<NoteDirection>(value%4));
        game.update(NotesInput::Right,now+=249,false);
        assert(game.activeDirection()!=NoteDirection::None);
        game.update(NotesInput::Confirm,++now,false);
        assert(game.activeDirection()==NoteDirection::None);
        game.update(NotesInput::Left,now+=450,false);
    }
    assert(game.phase()==NotesPhase::Answer && game.progress()==0);
}
int main() {
    // Every score tier, each direction, repeated identical notes and millis wrap.
    for (value=0;value<4;++value) for(unsigned score=0;score<=5;++score) {
        MemoryNotes game(randomNote); game.begin();
        uint32_t now=UINT32_MAX-300;
        const unsigned before=calls;
        game.update(NotesInput::Confirm,now,true);
        for(unsigned round=1;round<=5;++round) {
            assert(game.round()==round && game.length()==round+1);
            playback(game,now);
            const auto correct=static_cast<NotesInput>(value+1);
            // A stick held from playback, including its repeats, cannot answer.
            game.update(correct,++now,false);
            game.update(correct,now+=400,false);
            assert(game.progress()==0);
            game.update(NotesInput::None,++now,true);
            if(round<=score) {
                for(unsigned i=0;i<game.length();++i) {
                    game.update(correct,++now,false);
                    assert(game.progress()==i+1);
                    if(i+1<game.length()) {
                        game.update(correct,now+=200,false);
                        assert(game.progress()==i+1);
                        game.update(NotesInput::None,++now,true);
                    }
                }
                assert(game.phase()==NotesPhase::AnswerComplete && !game.rewardPending());
                assert(game.score()==round-1 && game.activeDirection()!=NoteDirection::None);
                game.update(NotesInput::Right,now+=179,false);
                assert(game.phase()==NotesPhase::AnswerComplete);
                game.update(NotesInput::Confirm,++now,true,false);
                assert(game.phase()==NotesPhase::AnswerComplete && !game.rewardPending());
                game.update(NotesInput::Confirm,++now,true,true);
            } else game.update(static_cast<NotesInput>((value+1)%4+1),++now,false);
            assert(game.phase()==NotesPhase::Feedback);
            assert(game.roundSucceeded()==(round<=score));
            assert(game.rewardPending()==(round==5));
            if(round<5) assert(game.takeReward()==0);
            game.update(NotesInput::Confirm,now+=799,true);
            assert(game.phase()==NotesPhase::Feedback);
            game.update(NotesInput::Confirm,++now,true);
        }
        assert(calls-before==20 && game.score()==score && game.phase()==NotesPhase::Summary);
        assert(game.takeReward()==(score==5?10:score==4?8:score>=2?6:3));
        assert(game.expReward()==(score>=4?15:score>=2?10:5));
        assert(game.takeReward()==0);
        game.update(NotesInput::Confirm,++now,true);
        assert(game.phase()==NotesPhase::Replay && !game.replaySelected());
        game.update(NotesInput::Up,++now,false);
        game.update(NotesInput::Confirm,++now,true);
        assert(game.phase()==NotesPhase::Playback && game.round()==1 && game.score()==0);
        assert(game.takeReward()==0);
        game.update(NotesInput::Cancel,++now,true);
        assert(game.phase()==NotesPhase::Inactive && game.takeReward()==0);
    }
    // Timeout is per next answer; exact deadline rejects even a correct input.
    value=0;
    MemoryNotes game(randomNote); uint32_t now=0;
    game.begin(); game.update(NotesInput::Confirm,now,true); playback(game,now);
    game.update(NotesInput::None,now+4999,true);
    assert(game.phase()==NotesPhase::Answer);
    game.update(NotesInput::Left,now+=5000,false);
    assert(game.phase()==NotesPhase::Feedback && game.timedOut() && game.score()==0);
    game.update(NotesInput::Confirm,now+=800,true); playback(game,now);
    game.update(NotesInput::None,++now,true);
    game.update(NotesInput::Left,now+=4998,false);
    assert(game.progress()==1);
    game.update(NotesInput::None,now+=4999,true);
    assert(game.phase()==NotesPhase::Answer);
    game.update(NotesInput::Left,++now,false);
    assert(game.timedOut());
    // Late ticks advance only one step, preserving the next note's duration.
    game.begin(); game.update(NotesInput::Confirm,++now,true);
    game.update(NotesInput::None,now+=9000,true);
    assert(game.phase()==NotesPhase::Playback && game.activeDirection()==NoteDirection::None);
    game.update(NotesInput::None,now+=450,true);
    assert(game.activeDirection()==NoteDirection::Left);
    game.update(NotesInput::Cancel,++now,true);
    assert(game.takeReward()==0);
    puts("PASS: memory notes scores 0-5, four directions, repeat/neutral gating, playback timing, timeout boundaries, wrap, replay and cancellation.");
}
