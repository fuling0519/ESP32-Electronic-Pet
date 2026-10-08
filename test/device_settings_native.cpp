#include <assert.h>
#include <stdio.h>
#include "storage/DeviceSettings.h"
FakeNvs nvs;
int main() {
    Storage::DeviceSettings settings;
    assert(!settings.saveVolume(1));
    assert(settings.init() && settings.volume()==1 && nvs.writes==0);
#if defined(PET_DEEP_SLEEP_TEST_MODE)
    assert(nvs.lastNamespace=="pet-prefs-test");
#else
    assert(nvs.lastNamespace=="pet-prefs");
#endif
    for(uint8_t level=0;level<2;++level) {
        assert(settings.saveVolume(level));
        const unsigned writes=nvs.writes;
        assert(settings.saveVolume(level) && nvs.writes==writes);
        Storage::DeviceSettings reboot;
        assert(reboot.init() && reboot.volume()==level);
    }
    assert(!settings.saveVolume(2) && settings.volume()==1);
    nvs.failWrite=nvs.writes+1;
    assert(!settings.saveVolume(0) && settings.volume()==1);
    Storage::DeviceSettings intact;
    assert(intact.init() && intact.volume()==1);
    nvs.failWrite=nvs.writes+1; nvs.torn=true;
    assert(!settings.saveVolume(0));
    Storage::DeviceSettings torn;
    assert(torn.init() && torn.volume()==1);
    nvs.failWrite=0;
    const unsigned beforeRepair=nvs.writes;
    assert(settings.saveVolume(1) && nvs.writes==beforeRepair+1);
    Storage::DeviceSettings repaired;
    assert(repaired.init() && repaired.volume()==1);
    assert(torn.saveVolume(1));
    nvs.bytes["volume"][2]=9;
    Storage::DeviceSettings invalid;
    assert(invalid.init() && invalid.volume()==1);
    nvs.bytes["volume"]={0x56,1,1,0};
    Storage::DeviceSettings checksum;
    assert(checksum.init() && checksum.volume()==1);
    nvs.bytes["volume"]={0x56,3,1,254};
    Storage::DeviceSettings version;
    assert(version.init() && version.volume()==1);
    // All valid legacy levels map to the two new modes without boot writes.
    for(uint8_t old=0;old<4;++old) {
        nvs.bytes["volume"]={0x56,1,old,static_cast<uint8_t>(~old)};
        const unsigned writes=nvs.writes;
        Storage::DeviceSettings legacy;
        assert(legacy.init() && legacy.volume()==(old?1:0) && nvs.writes==writes);
        assert(legacy.saveVolume(legacy.volume()) && nvs.writes==writes+1);
        assert(nvs.bytes["volume"][1]==2);
    }
    nvs.bytes["volume"]={0x56,2,2,253};
    Storage::DeviceSettings invalidBinary;
    assert(invalidBinary.init() && invalidBinary.volume()==1);
    nvs.failBegin=true;
    Storage::DeviceSettings unavailable;
    assert(!unavailable.init() && unavailable.volume()==1 && !unavailable.saveVolume(1));
    puts("PASS: independent volume persistence, unchanged writes, reboot, invalid/version/torn records and failed writes.");
}
