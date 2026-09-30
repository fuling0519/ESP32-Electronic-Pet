#pragma once

namespace Hardware {
// Public identifier, never an authentication credential. Empty on read failure.
const char* deviceId();
}
