#pragma once

#include <stdint.h>

#include "pet/PetData.h"

namespace Hardware {
class Display;
}

namespace Ui {
namespace PetIcons {
// Art contract for future species. Home reserves x=20..108 / y=0..54;
// a 64 x 44 sprite at x=32, y=6 is the recommended standard canvas.
constexpr uint8_t kHomePetCanvasWidth = 64;
constexpr uint8_t kHomePetCanvasHeight = 44;
constexpr uint8_t kGhostFrameWidth = 30;
constexpr uint8_t kGhostFrameHeight = 28;

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
// Dissolve stages 0..3 progressively remove the temporary slime.
void drawPetPlaceholderDissolve(Hardware::Display& display, int16_t x, int16_t y,
                                uint8_t stage);
// Shared user-drawn ghost. Frames 0 and 1 alternate the wing positions.
void drawGhost(Hardware::Display& display, int16_t x, int16_t y,
               uint8_t frame);
void drawTombstone(Hardware::Display& display, int16_t x, int16_t y);
}  // namespace PetIcons
}  // namespace Ui
