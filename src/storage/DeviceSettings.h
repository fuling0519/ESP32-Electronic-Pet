#pragma once
#include <Preferences.h>
#include <stdint.h>

namespace Storage {
// Device preferences are independent of all pet and memorial snapshots.
class DeviceSettings {
public:
    bool init();
    uint8_t volume() const { return volume_; }
    bool saveVolume(uint8_t level);
private:
    Preferences preferences_;
    bool initialized_ = false;
    bool valid_ = false;
    uint8_t volume_ = 1;
};
}
