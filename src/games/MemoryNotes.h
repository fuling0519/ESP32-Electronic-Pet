#pragma once
#include <stdint.h>

namespace Games {
enum class NoteDirection : uint8_t { Left, Up, Right, Down, None };
enum class NotesPhase : uint8_t { Inactive, Ready, Playback, Answer, AnswerComplete, Feedback, Summary, Replay };
enum class NotesInput : uint8_t { None, Left, Up, Right, Down, Confirm, Cancel };

// Species-independent rules. The application owns pet mutations and sound.
class MemoryNotes {
public:
    using RandomSource = uint32_t (*)();
    explicit MemoryNotes(RandomSource random) : random_(random) {}
    static constexpr uint8_t kRounds = 5;
    static constexpr uint32_t kToneMs = 250, kGapMs = 450;
    static constexpr uint32_t kAnswerFlashMs = 180, kAnswerTimeoutMs = 5000;
    static constexpr uint32_t kFeedbackMs = 800;
    void begin();
    void cancel();
    bool update(NotesInput input, uint32_t now, bool neutral, bool soundFinished = true);
    NotesPhase phase() const { return phase_; }
    uint8_t round() const { return round_; }
    uint8_t length() const { return round_ + 1; }
    uint8_t score() const { return score_; }
    uint8_t progress() const { return phase_ == NotesPhase::Playback ? playbackIndex_ + 1 : answered_; }
    NoteDirection activeDirection() const;
    NoteDirection expectedDirection() const { return sequence_[answered_ < length() ? answered_ : length()-1]; }
    bool roundSucceeded() const { return roundSucceeded_; }
    bool timedOut() const { return timedOut_; }
    bool replaySelected() const { return replaySelected_; }
    bool rewardPending() const { return rewardPending_; }
    uint8_t takeReward();
    uint8_t expReward() const;
private:
    void startRound(uint32_t now);
    void finishRound(bool success, bool timeout, uint32_t now);
    RandomSource random_;
    NotesPhase phase_ = NotesPhase::Inactive;
    NoteDirection sequence_[6]{};
    NoteDirection answerDirection_ = NoteDirection::None;
    uint8_t round_ = 0, score_ = 0, playbackIndex_ = 0, answered_ = 0;
    uint32_t phaseStartedAt_ = 0, answerStartedAt_ = 0, flashStartedAt_ = 0;
    bool toneOn_ = false, armed_ = false;
    bool roundSucceeded_ = false, timedOut_ = false;
    bool replaySelected_ = false, rewardPending_ = false;
};
}  // namespace Games
