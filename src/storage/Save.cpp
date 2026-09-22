#include "storage/Save.h"

namespace Storage {
bool Save::init() { initialized_ = true; return initialized_; }
bool Save::isInitialized() const { return initialized_; }
}  // namespace Storage
