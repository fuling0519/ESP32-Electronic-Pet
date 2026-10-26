#pragma once
#include <stddef.h>
#include <stdint.h>
#include "pet/PetData.h"
#include "storage/Memorials.h"

namespace Ble {

// pet_event (feature pet_events): best-effort hints for the browser's animations only.
enum class PetEventType : uint8_t {
    Fed, Cleaned, Treated, Slept, Woke, Hatched, Grew, LevelUp, GotSick, Died,
    Departed, Adopted, MemorialAdded, MemorialDeleted,
};
struct PetEvent {
    PetEventType type = PetEventType::Fed;
    uint64_t petId = 0;          // Current pet; the new pet for Adopted.
    uint8_t level = 0;           // LevelUp only.
    uint64_t memorialPetId = 0;  // Memorial events only.
    bool fromWeb = false;        // Player actions report source device or web.
};
const char* petEventName(PetEventType type);
bool makePetEventJson(const PetEvent& event, char* out, size_t cap, uint16_t& len);

// Events never wait for a connection and never take the last slots of the transmit
// queue, so status snapshots and command replies are not displaced.
constexpr uint8_t kEventReservedSlots = 2;
inline bool petEventAdmitted(bool connected, bool subscribed, uint8_t queued, uint8_t capacity) {
    return connected && subscribed && queued + kEventReservedSlots < capacity;
}

// State events from comparing successive observations of the pet and memorial archive.
// Care and sleep events come from the application's successful UiAction results instead.
class PetEventDetector {
public:
    void reset(const Pet::PetData& pet, const Storage::Memorials& memorials);
    // Writes up to cap events (extras are dropped) and records the new observation.
    uint8_t detect(const Pet::PetData& pet, const Storage::Memorials& memorials, PetEvent* out, uint8_t cap);
    // A web command caused this change: the next detect() reports events of this type with
    // source web, then forgets the mark.
    void markFromWeb(PetEventType type) { webMask_ |= static_cast<uint16_t>(1u << static_cast<uint8_t>(type)); }

private:
    bool ready_ = false;
    uint16_t webMask_ = 0;
    uint64_t petId_ = 0;
    Pet::LifeStage stage_ = Pet::LifeStage::Egg;
    uint8_t level_ = 0;
    bool sick_ = false, dead_ = false, departed_ = false;
    uint8_t memorialCount_ = 0;
    uint64_t memorialIds_[Storage::kMemorialCapacity]{};
};

}  // namespace Ble
