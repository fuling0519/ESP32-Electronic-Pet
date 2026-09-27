#pragma once

#include <Preferences.h>
#include <stdint.h>

#include "pet/PetData.h"

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
    LoadStatus load(Pet::PetData& pet);
    bool save(const Pet::PetData& pet);
    void markDirty(uint32_t nowMs);
    void update(const Pet::PetData& pet, uint32_t nowMs);

private:
    static constexpr uint32_t kDeferredSaveMs = 2000;
    static constexpr uint32_t kMinimumSaveIntervalMs = 10000;
    static constexpr uint32_t kPeriodicSaveIntervalMs = 5UL * 60 * 1000;

    Preferences preferences_;
    bool initialized_ = false;
    bool writesBlocked_ = false;
    bool hasActiveSlot_ = false;
    bool activeSlotA_ = false;
    bool dirty_ = false;
    uint32_t sequence_ = 0;
    uint32_t dirtySinceMs_ = 0;
    uint32_t lastSaveAtMs_ = 0;
};

}  // namespace Storage
