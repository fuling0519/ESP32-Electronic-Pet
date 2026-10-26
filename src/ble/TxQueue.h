#pragma once
#include <stdint.h>
#include "ble/Messages.h"

namespace Ble {

// Status snapshots are resent by the link, events are best effort, replies (command_result,
// action_result, game messages) must not be lost.
enum class TxKind : uint8_t { Status, Event, Reply };

// Outgoing messages waiting for the notify characteristic, oldest first.
// A status replaces a queued status; an event leaves kEventReservedSlots free; a full queue makes
// room for a reply by dropping the newest event, then the queued status. A reply is refused only
// when all slots already hold replies.
class TxQueue {
public:
    static constexpr uint8_t kCapacity = 8;
    struct Message { uint16_t length = 0; TxKind kind = TxKind::Reply; char json[kMaxMessageBytes + 1]; };
    // statusDropped: set when a queued status made room, so the link can send it again.
    bool push(const char* json, uint16_t length, TxKind kind, bool* statusDropped = nullptr);
    bool pop(Message& out);
    void clear() { count_ = 0; }
    uint8_t count() const { return count_; }
    TxKind kindAt(uint8_t i) const { return slots_[i].kind; }
private:
    void removeAt(uint8_t i);
    Message slots_[kCapacity];
    uint8_t count_ = 0;
};

}  // namespace Ble
