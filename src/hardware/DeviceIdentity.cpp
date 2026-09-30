#include "hardware/DeviceIdentity.h"
#include <esp_system.h>
#include <stdint.h>
#include <stdio.h>

namespace Hardware {
const char* deviceId() {
    static char id[23]{};  // esp32-pet- + 12 uppercase hex digits + NUL
    if (id[0]) return id;
    uint8_t mac[6]{};
    // Factory eFuse survives reboot, firmware updates and NVS erasure.
    if (esp_efuse_mac_get_default(mac) != ESP_OK) return id;
    bool allZero = true, allFF = true;
    for (uint8_t byte : mac) {
        allZero = allZero && byte == 0;
        allFF = allFF && byte == 0xff;
    }
    if (allZero || allFF) return id;
    snprintf(id, sizeof(id), "esp32-pet-%02X%02X%02X%02X%02X%02X",
             static_cast<unsigned>(mac[0]), static_cast<unsigned>(mac[1]),
             static_cast<unsigned>(mac[2]), static_cast<unsigned>(mac[3]),
             static_cast<unsigned>(mac[4]), static_cast<unsigned>(mac[5]));
    return id;
}
}
