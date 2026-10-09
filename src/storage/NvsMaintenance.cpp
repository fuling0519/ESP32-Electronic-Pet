// Dedicated maintenance firmware. It never starts or rewrites the pet.
// Back up Flash and inspect the inventory before sending CLEAR_OLD_TESTS.
#if defined(PET_NVS_MAINTENANCE_MODE)
#include <Arduino.h>
#include <nvs.h>
#include <string.h>

namespace {
// Fixed allowlist of superseded tests. Formal saves, preferences, both current
// wyvern tests, calibration, network settings and unknown namespaces stay intact.
const char* const kOldTests[] = {
    "pet-deep", "pet-grow", "pet-mem", "pet-sleep", "pet-test",
    "pet-sad-a", "pet-sad-b", "pet-treat-a", "pet-treat-b"
};
bool finished = false;
char command[32]{};
uint8_t commandLength = 0;
uint32_t lastStatus = 0;

void report() {
    nvs_stats_t stats{};
    const auto result = nvs_get_stats(nullptr, &stats);
    Serial.printf("NVS_STATS err=%d used=%u free=%u total=%u namespaces=%u\n",
        result, static_cast<unsigned>(stats.used_entries), static_cast<unsigned>(stats.free_entries),
        static_cast<unsigned>(stats.total_entries), static_cast<unsigned>(stats.namespace_count));
}

void clearOldTests() {
    if (finished) { Serial.println("ALREADY_FINISHED"); return; }
    report();
    for (const char* name : kOldTests) {
        nvs_handle_t handle;
        auto err = nvs_open(name, NVS_READONLY, &handle);
        if (err == ESP_ERR_NVS_NOT_FOUND) { Serial.printf("SKIP %s absent\n", name); continue; }
        if (err != ESP_OK) { Serial.printf("ABORT open %s err=%d\n", name, err); return; }
        size_t count = 0;
        err = nvs_get_used_entry_count(handle, &count);
        nvs_close(handle);
        if (err != ESP_OK) { Serial.printf("ABORT count %s err=%d\n", name, err); return; }
        if (!count) { Serial.printf("SKIP %s empty\n", name); continue; }
        err = nvs_open(name, NVS_READWRITE, &handle);
        if (err != ESP_OK) { Serial.printf("ABORT write-open %s err=%d\n", name, err); return; }
        err = nvs_erase_all(handle);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
        if (err != ESP_OK) { Serial.printf("ABORT erase %s err=%d\n", name, err); return; }
        Serial.printf("CLEARED %s entries=%u\n", name, static_cast<unsigned>(count));
    }
    finished = true;
    Serial.println("CLEANUP_FINISHED");
    report();
}
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("NVS_MAINTENANCE_READY: inspection only until CLEAR_OLD_TESTS");
    report();
}

void loop() {
    while (Serial.available()) {
        const char ch = Serial.read();
        if (ch == '\r') continue;
        if (ch == '\n') {
            command[commandLength] = 0;
            if (!strcmp(command, "CLEAR_OLD_TESTS")) clearOldTests();
            else if (!strcmp(command, "STATUS")) report();
            else Serial.println("UNKNOWN_COMMAND");
            commandLength = 0;
        } else if (commandLength + 1 < sizeof(command)) command[commandLength++] = ch;
        else commandLength = 0;
    }
    if (millis() - lastStatus >= 5000) {
        lastStatus = millis();
        Serial.println(finished ? "MAINTENANCE_FINISHED" : "MAINTENANCE_WAITING");
    }
    delay(5);
}
#endif
