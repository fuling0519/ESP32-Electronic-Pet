#pragma once
#include "pet/PetData.h"

namespace Power {
// RTC state is only accepted after a genuine deep-sleep reset, with a valid
// checksum, matching build mode and matching NVS checkpoint.
void releaseWakePins();
bool retainedCalibration(int& x, int& y);
bool resume(Pet::PetData& pet, uint32_t& remainderMs);
bool configure();
bool wakePressed();
void cancel();
void retain(const Pet::PetData& pet, int x, int y, uint32_t remainderMs);
void enter();
}
