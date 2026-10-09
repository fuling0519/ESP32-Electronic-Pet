#pragma once

#include <Preferences.h>
#include <stdint.h>

#include "pet/PetData.h"
#include "storage/Memorials.h"
#include "storage/SaveFormat.h"

namespace Storage {

enum class LoadStatus : uint8_t {
    Loaded,
    Empty,
    Invalid,
    Unavailable,
};

class Save {
public:
    bool init();
    bool isInitialized() const;
    bool canWriteMemorials() const { return initialized_ && !memorialWritesBlocked_ && !writesBlocked_; }
    LoadStatus load(Pet::PetData& pet);
    bool save(const Pet::PetData& pet);
    LoadStatus loadMemorials(Memorials& memorials);
    bool saveMemorials(const Memorials& memorials);
    bool appendMemorial(const Pet::PetData& pet, Memorials& memorials);
    bool removeMemorial(uint8_t index, Memorials& memorials);
    void markDirty(uint32_t nowMs);
    void update(const Pet::PetData& pet, uint32_t nowMs);
    bool hasSaveFailure() const { return saveFailures_ != 0; }
    bool capacityLow() const { return capacityKnown_ && freeEntries_ < kCapacityReserveEntries; }
    uint32_t retryDelayMs() const { return retryDelayMs_; }
    uint32_t freeEntries() const { return freeEntries_; }
    void refreshCapacity();

private:
    static constexpr uint32_t kDeferredSaveMs = 2000;
    static constexpr uint32_t kMinimumSaveIntervalMs = 10000;
    static constexpr uint32_t kPeriodicSaveIntervalMs = 5UL * 60 * 1000;
    // Keep headroom for a page of collection work and a complete archive blob.
    static constexpr uint32_t kCapacityReserveEntries = 160;
    bool failedSaveAttempt();

    // Owned by the application, outside the 8KB Arduino loop-task stack.
    // All persistence runs sequentially on that task; BLE callbacks never write.
    uint8_t memorialBytes_[kMemorialSaveRecordSize]{};
    Memorials memorialScratchA_;
    Memorials memorialScratchB_;
    Preferences preferences_;
    bool initialized_ = false;
    bool writesBlocked_ = false;
    bool hasActiveSlot_ = false;
    bool activeSlotA_ = false;
    bool dirty_ = false;
    uint32_t sequence_ = 0;
    uint32_t dirtySinceMs_ = 0;
    uint32_t lastSaveAtMs_ = 0;
    uint32_t lastSaveAttemptAtMs_ = 0;
    uint32_t retryDelayMs_ = 0;
    uint8_t saveFailures_ = 0;
    bool capacityKnown_ = false;
    uint32_t freeEntries_ = 0;
    bool memorialWritesBlocked_ = false;
    bool hasActiveMemorialSlot_ = false;
    bool activeMemorialSlotA_ = false;
    uint32_t memorialSequence_ = 0;
};

}  // namespace Storage
