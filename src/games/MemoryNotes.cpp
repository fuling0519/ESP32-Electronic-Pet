#include "games/MemoryNotes.h"

namespace Games {
void MemoryNotes::begin() {
    phase_ = NotesPhase::Ready;
    round_ = score_ = answered_ = playbackIndex_ = 0;
    toneOn_ = armed_ = roundSucceeded_ = timedOut_ = replaySelected_ = rewardPending_ = false;
    answerDirection_ = NoteDirection::None;
}
void MemoryNotes::cancel() {
    phase_ = NotesPhase::Inactive;
    rewardPending_ = toneOn_ = false;
    answerDirection_ = NoteDirection::None;
}
void MemoryNotes::startRound(uint32_t now) {
    ++round_;
    for (uint8_t i = 0; i < length(); ++i)
        sequence_[i] = static_cast<NoteDirection>(random_() % 4);
    playbackIndex_ = answered_ = 0;
    phase_ = NotesPhase::Playback;
    phaseStartedAt_ = now;
    toneOn_ = true;
    armed_ = roundSucceeded_ = timedOut_ = false;
    answerDirection_ = NoteDirection::None;
}
void MemoryNotes::finishRound(bool success, bool timeout, uint32_t now) {
    roundSucceeded_ = success;
    timedOut_ = timeout;
    if (success) ++score_;
    phase_ = NotesPhase::Feedback;
    phaseStartedAt_ = now;
    rewardPending_ = round_ == kRounds;
}
NoteDirection MemoryNotes::activeDirection() const {
    if (phase_ == NotesPhase::Playback && toneOn_) return sequence_[playbackIndex_];
    if (phase_ == NotesPhase::Answer || phase_ == NotesPhase::AnswerComplete) return answerDirection_;
    return NoteDirection::None;
}
uint8_t MemoryNotes::takeReward() {
    if (!rewardPending_) return 0;
    rewardPending_ = false;
    return score_ == 5 ? 10 : score_ == 4 ? 8 : score_ >= 2 ? 6 : 3;
}
uint8_t MemoryNotes::expReward() const { return score_ >= 4 ? 15 : score_ >= 2 ? 10 : 5; }

bool MemoryNotes::update(NotesInput input, uint32_t now, bool neutral, bool soundFinished) {
    if (phase_ == NotesPhase::Inactive) return false;
    if (input == NotesInput::Cancel) { cancel(); return true; }
    if (phase_ == NotesPhase::Ready) {
        if (input == NotesInput::Confirm) { startRound(now); return true; }
        return false;
    }
    if (phase_ == NotesPhase::Playback) {
        // Give every note a full visible duration, even after a delayed loop.
        const uint32_t duration = toneOn_ ? kToneMs : kGapMs;
        if (now - phaseStartedAt_ < duration) return false;
        phaseStartedAt_ = now;
        if (toneOn_) toneOn_ = false;
        else if (++playbackIndex_ < length()) toneOn_ = true;
        else {
            phase_ = NotesPhase::Answer;
            answerStartedAt_ = now;
            armed_ = false; // Consume transition input; first observe neutral.
        }
        return true;
    }
    if (phase_ == NotesPhase::AnswerComplete) {
        // Keep the final answer visible until its tone has actually finished.
        // Consume inputs here; feedback starts its own reading interval.
        if (now - flashStartedAt_ < kAnswerFlashMs || !soundFinished) return false;
        finishRound(true, false, now);
        return true;
    }
    if (phase_ == NotesPhase::Answer) {
        if (now - answerStartedAt_ >= kAnswerTimeoutMs) {
            answerDirection_ = NoteDirection::None;
            finishRound(false, true, now);
            return true;
        }
        bool changed = false;
        if (answerDirection_ != NoteDirection::None && now - flashStartedAt_ >= kAnswerFlashMs) {
            answerDirection_ = NoteDirection::None;
            changed = true;
        }
        if (neutral) armed_ = true;
        const bool direction = input >= NotesInput::Left && input <= NotesInput::Down;
        if (direction && armed_ && !neutral) {
            armed_ = false;
            answerDirection_ = static_cast<NoteDirection>(static_cast<uint8_t>(input)-1);
            flashStartedAt_ = now;
            if (answerDirection_ != sequence_[answered_]) finishRound(false, false, now);
            else {
                ++answered_;
                answerStartedAt_ = now;
                if (answered_ == length()) phase_ = NotesPhase::AnswerComplete;
            }
            return true;
        }
        return changed;
    }
    if (phase_ == NotesPhase::Feedback) {
        if (input == NotesInput::Confirm && now - phaseStartedAt_ >= kFeedbackMs) {
            if (round_ == kRounds) phase_ = NotesPhase::Summary;
            else startRound(now);
            return true;
        }
        return false;
    }
    if (phase_ == NotesPhase::Summary) {
        if (input == NotesInput::Confirm) {
            phase_ = NotesPhase::Replay;
            replaySelected_ = false;
            return true;
        }
        return false;
    }
    if (phase_ == NotesPhase::Replay) {
        if (input == NotesInput::Up || input == NotesInput::Down) {
            replaySelected_ = !replaySelected_;
            return true;
        }
        if (input == NotesInput::Confirm) {
            if (replaySelected_) { begin(); startRound(now); }
            else cancel();
            return true;
        }
    }
    return false;
}
}  // namespace Games
