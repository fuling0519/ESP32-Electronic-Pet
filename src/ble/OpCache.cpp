#include "ble/OpCache.h"

namespace Ble {

const OpCache::Entry* OpCache::find(uint64_t op) const {
    for (const Entry& e : entries_) if (e.used && e.op == op) return &e;
    return nullptr;
}

OpCache::Match OpCache::check(uint64_t op, OpKind kind, uint8_t action, uint64_t target, uint32_t commandId) {
    Entry* e = const_cast<Entry*>(find(op));
    if (!e) return Match::New;
    if (e->kind != kind || e->action != action || e->target != target) return Match::Conflict;
    e->commandId = commandId;
    return e->done ? Match::Done : Match::Pending;
}

void OpCache::begin(uint64_t op, OpKind kind, uint8_t action, uint64_t target, uint32_t commandId) {
    Entry* slot = const_cast<Entry*>(find(op));
    if (!slot) for (Entry& e : entries_) if (!e.used) { slot = &e; break; }
    if (!slot) {
        // Full: the oldest finished entry goes first, so a pending op keeps its slot.
        for (Entry& e : entries_)
            if (e.done && (!slot || e.order < slot->order)) slot = &e;
        if (!slot) {
            slot = &entries_[0];
            for (Entry& e : entries_) if (e.order < slot->order) slot = &e;
        }
    }
    Entry e;
    e.op = op; e.target = target; e.commandId = commandId; e.order = ++order_;
    e.kind = kind; e.action = action; e.used = true;
    *slot = e;
}

const OpCache::Entry* OpCache::complete(uint64_t op, const CareOutcome& outcome) {
    Entry* e = const_cast<Entry*>(find(op));
    if (!e) return nullptr;
    e->done = true; e->outcome = outcome;
    return e;
}

const OpCache::Entry* OpCache::complete(uint64_t op, const LifecycleOutcome& outcome) {
    Entry* e = const_cast<Entry*>(find(op));
    if (!e) return nullptr;
    e->done = true; e->lifecycle = outcome;
    return e;
}

const OpCache::Entry* OpCache::complete(uint64_t op, const MemorialOutcome& outcome) {
    Entry* e = const_cast<Entry*>(find(op));
    if (!e) return nullptr;
    e->done = true; e->memorial = outcome;
    return e;
}

const OpCache::Entry* OpCache::complete(uint64_t op, const SoundOutcome& outcome) {
    Entry* e = const_cast<Entry*>(find(op));
    if (!e) return nullptr;
    e->done = true; e->sound = outcome;
    return e;
}

}  // namespace Ble
