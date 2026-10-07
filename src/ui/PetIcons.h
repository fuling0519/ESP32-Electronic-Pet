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
void drawDirt(Hardware::Display& display, Pet::CleanlinessState state);
void drawSick(Hardware::Display& display, int16_t x, int16_t y);
void drawStatusCard(Hardware::Display& display, int16_t x, int16_t y);
// User-drawn egg, baby and adult sheets, each with two 64 x 44 frames.
void drawPet(Hardware::Display& display, int16_t x, int16_t y,
             Pet::LifeStage lifeStage, uint8_t eggCrackStage, uint8_t frame);
void drawMemorialPet(Hardware::Display& display, Pet::LifeStage stage, bool resting = false);
void drawCenteredPostcardPet(Hardware::Display& display, Pet::LifeStage stage);
void drawSadPet(Hardware::Display& display, int16_t x, int16_t y,
                Pet::LifeStage lifeStage, uint8_t frame);
// Shared effect; origin is independent of species and sprite bounds.
void drawSadLines(Hardware::Display& display, int16_t x, int16_t y);
// Draws a frame generated from the current bird with its eyes closed.
void drawSleepingPet(Hardware::Display& display, int16_t x, int16_t y,
                     Pet::LifeStage lifeStage, uint8_t frame);
void drawEatingPet(Hardware::Display& display, int16_t x, int16_t y,
                   Pet::LifeStage lifeStage, uint8_t frame, uint8_t bowlStage);
// Dissolve stages 0..3 progressively remove the current bird.
void drawPetDissolve(Hardware::Display& display, int16_t x, int16_t y,
                     Pet::LifeStage lifeStage, uint8_t dissolveStage);
// Shared user-drawn ghost. Frames 0 and 1 alternate the wing positions.
void drawGhost(Hardware::Display& display, int16_t x, int16_t y,
               uint8_t frame);
// Draws a scalable tombstone outline. width/height include the wider base.
void drawTombstone(Hardware::Display& display, int16_t x, int16_t y,
                   uint8_t width = 42, uint8_t height = 47);
}  // namespace PetIcons
}  // namespace Ui
