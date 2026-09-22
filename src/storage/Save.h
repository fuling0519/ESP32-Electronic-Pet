#pragma once

namespace Storage {
// Data schema and Preferences/NVS implementation await a defined pet state.
class Save {
public:
    bool init();
    bool isInitialized() const;
private:
    bool initialized_ = false;
};
}  // namespace Storage
