#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

// Host-only NVS model. A torn write damages only the target key; failures can
// be placed between the pet checkpoint, archive and adoption writes.
struct FakeNvs {
    std::map<std::string, std::vector<uint8_t>> bytes;
    unsigned writes = 0;
    unsigned failWrite = 0;
    bool torn = false;
    bool failBegin = false;
    bool failAllWrites = false;
    bool failStats = false;
    size_t freeEntries = 500;
    std::string lastNamespace;
};
extern FakeNvs nvs;
class Preferences {
public:
    bool begin(const char* name, bool) { nvs.lastNamespace=name; return !nvs.failBegin; }
    size_t getBytesLength(const char* key) { return nvs.bytes[key].size(); }
    size_t getBytes(const char* key, void* out, size_t size) {
        const auto& value = nvs.bytes[key];
        const size_t copied = std::min(size, value.size());
        if (copied) std::memcpy(out, value.data(), copied);
        return copied;
    }
    size_t putBytes(const char* key, const void* data, size_t size) {
        ++nvs.writes;
        const auto* source = static_cast<const uint8_t*>(data);
        if (nvs.failAllWrites || nvs.writes == nvs.failWrite) {
            if (nvs.torn) nvs.bytes[key] = std::vector<uint8_t>(source, source + size / 2);
            return 0;
        }
        nvs.bytes[key] = std::vector<uint8_t>(source, source + size);
        return size;
    }
};
