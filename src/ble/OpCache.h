#pragma once
#include <stdint.h>
#include "ble/CareCommand.h"
#include "ble/LifecycleCommand.h"

namespace Ble {

// Write commands that carry an op (delete_memorial later). action: CareAction or LifecycleAction.
enum class OpKind : uint8_t { Care, Lifecycle };

// The last 8 accepted ops, in RAM only. Rejected commands are never recorded, so a retry
// with the same op is checked again from scratch.
class OpCache {
public:
    static constexpr uint8_t kCapacity = 8;
    enum class Match : uint8_t { New, Pending, Done, Conflict };
    struct Entry {
        uint64_t op = 0, target = 0;
        uint32_t commandId = 0;  // Latest accepted command id: the next action_result uses it.
        uint32_t order = 0;
        OpKind kind = OpKind::Care;
        uint8_t action = 0;
        bool used = false, done = false;
        CareOutcome outcome{};          // Care
        LifecycleOutcome lifecycle{};   // Lifecycle
    };
    // A pending or done match takes commandId; another command, action or target is a conflict.
    Match check(uint64_t op, OpKind kind, uint8_t action, uint64_t target, uint32_t commandId);
    // Records an accepted op as pending, replacing the oldest finished entry (or the oldest).
    void begin(uint64_t op, OpKind kind, uint8_t action, uint64_t target, uint32_t commandId);
    // Marks it done; null when the op was evicted meanwhile.
    const Entry* complete(uint64_t op, const CareOutcome& outcome);
    const Entry* complete(uint64_t op, const LifecycleOutcome& outcome);
    const Entry* find(uint64_t op) const;
private:
    Entry entries_[kCapacity]{};
    uint32_t order_ = 0;
};

}  // namespace Ble
