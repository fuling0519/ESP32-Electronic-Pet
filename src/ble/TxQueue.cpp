#include "ble/TxQueue.h"
#include <string.h>
#include "ble/PetEvents.h"

namespace Ble {

void TxQueue::removeAt(uint8_t i) {
    for (; i + 1 < count_; ++i) {
        Message& to = slots_[i]; const Message& from = slots_[i + 1];
        to.length = from.length; to.kind = from.kind;
        memcpy(to.json, from.json, from.length + 1);
    }
    --count_;
}

bool TxQueue::push(const char* json, uint16_t length, TxKind kind, bool* statusDropped) {
    if (statusDropped) *statusDropped = false;
    if (!json || !length || length > kMaxMessageBytes) return false;
    Message* slot = nullptr;
    if (kind == TxKind::Status) {
        for (uint8_t i = 0; i < count_ && !slot; ++i) if (slots_[i].kind == TxKind::Status) slot = &slots_[i];
    } else if (kind == TxKind::Event) {
        if (count_ + kEventReservedSlots >= kCapacity) return false;
    } else if (count_ >= kCapacity) {
        int8_t victim = -1;
        for (int8_t i = count_ - 1; i >= 0 && victim < 0; --i) if (slots_[i].kind == TxKind::Event) victim = i;
        for (int8_t i = count_ - 1; i >= 0 && victim < 0; --i) if (slots_[i].kind == TxKind::Status) victim = i;
        if (victim < 0) return false;
        if (slots_[victim].kind == TxKind::Status && statusDropped) *statusDropped = true;
        removeAt(static_cast<uint8_t>(victim));
    }
    if (!slot) {
        if (count_ >= kCapacity) return false;
        slot = &slots_[count_++];
    }
    slot->length = length; slot->kind = kind;
    memcpy(slot->json, json, length);
    slot->json[length] = '\0';
    return true;
}

bool TxQueue::pop(Message& out) {
    if (!count_) return false;
    const Message& m = slots_[0];
    out.length = m.length; out.kind = m.kind;
    memcpy(out.json, m.json, m.length + 1);
    removeAt(0);
    return true;
}

}  // namespace Ble
