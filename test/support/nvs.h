#pragma once
#include <stddef.h>
#include "Preferences.h"
typedef int esp_err_t;
constexpr esp_err_t ESP_OK = 0;
struct nvs_stats_t {
    size_t used_entries, free_entries, total_entries, namespace_count;
};
inline esp_err_t nvs_get_stats(const char*, nvs_stats_t* stats) {
    if (nvs.failStats) return -1;
    *stats = {630 - nvs.freeEntries, nvs.freeEntries, 630, 1};
    return ESP_OK;
}
