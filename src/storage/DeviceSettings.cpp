#include "storage/DeviceSettings.h"
#include "hardware/Sound.h"
#include <string.h>

namespace Storage {
namespace {
#if defined(PET_SAD_TEST_MODE) || defined(PET_TREATMENT_TEST_MODE) || defined(PET_DEEP_SLEEP_TEST_MODE) || defined(PET_DEATH_TEST_MODE) || defined(PET_MEMORIAL_TEST_MODE) || defined(PET_SLEEP_TEST_MODE) || defined(PET_GROWTH_TEST_MODE) || defined(PET_MEMORIAL_SAMPLE_MODE) || defined(PET_DIRTY_PREVIEW_MODE)
constexpr char kNamespace[] = "pet-prefs-test";
#else
constexpr char kNamespace[] = "pet-prefs";
#endif
constexpr char kKey[] = "volume";
}
bool DeviceSettings::init() {
    volume_ = Hardware::Sound::kDefaultVolume;
    valid_ = false;
    initialized_ = preferences_.begin(kNamespace, false);
    if (!initialized_) return false;
    uint8_t record[4]{};
    if (preferences_.getBytesLength(kKey) == sizeof(record) &&
        preferences_.getBytes(kKey, record, sizeof(record)) == sizeof(record) &&
        record[0] == 0x56 &&
        ((record[1] == 1 && record[2] < 4) ||
         (record[1] == 2 && record[2] < Hardware::Sound::kVolumeCount)) &&
        record[3] == static_cast<uint8_t>(~record[2])) {
        // Read legacy four-level settings without writing during boot.
        // Legacy mute remains muted; every audible level becomes enabled.
        volume_ = record[2] ? 1 : 0;
        valid_ = record[1] == 2;
    }
    return true;
}
bool DeviceSettings::saveVolume(uint8_t level) {
    if (!initialized_ || level >= Hardware::Sound::kVolumeCount) return false;
    if (valid_ && level == volume_) return true;
    // After an attempted write, trust the cache again only after readback.
    // A failed/torn write must not make a later repair look "unchanged".
    valid_ = false;
    const uint8_t record[] = {0x56, 2, level, static_cast<uint8_t>(~level)};
    if (preferences_.putBytes(kKey, record, sizeof(record)) != sizeof(record)) return false;
    uint8_t actual[sizeof(record)]{};
    if (preferences_.getBytesLength(kKey) != sizeof(record) ||
        preferences_.getBytes(kKey, actual, sizeof(actual)) != sizeof(actual) ||
        memcmp(actual, record, sizeof(record)) != 0) return false;
    volume_ = level;
    valid_ = true;
    return true;
}
}
