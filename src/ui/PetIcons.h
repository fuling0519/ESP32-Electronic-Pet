#pragma once

#include <stdint.h>

#include "pet/PetData.h"

namespace Hardware {
class Display;
}

namespace Ui {
namespace PetIcons {
// All coordinates are top-left origins. Need icons fit a 14 x 14 canvas.
void drawMood(Hardware::Display& display, int16_t x, int16_t y, Pet::MoodState state);
void drawHunger(Hardware::Display& display, int16_t x, int16_t y, Pet::HungerState state);
// Alert / shortcut icons fit a 12 x 12 canvas.
void drawCleaningAlert(Hardware::Display& display, int16_t x, int16_t y,
                       Pet::CleanlinessState state);
void drawSick(Hardware::Display& display, int16_t x, int16_t y);
void drawStatusCard(Hardware::Display& display, int16_t x, int16_t y);
// Temporary slime, 44 x 32. Replace this drawing for a future sprite.
void drawPetPlaceholder(Hardware::Display& display, int16_t x, int16_t y);
}  // namespace PetIcons
}  // namespace Ui
