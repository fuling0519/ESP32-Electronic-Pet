#include "ui/PetIcons.h"

#include <Arduino.h>

#include "hardware/Display.h"

namespace Ui {
namespace PetIcons {

void drawMood(Hardware::Display& d, int16_t x, int16_t y, Pet::MoodState state) {
    d.drawLine(x + 4, y + 1, x + 9, y + 1);
    d.drawLine(x + 1, y + 4, x + 4, y + 1);
    d.drawLine(x + 9, y + 1, x + 12, y + 4);
    d.drawLine(x + 1, y + 4, x + 1, y + 9);
    d.drawLine(x + 12, y + 4, x + 12, y + 9);
    d.drawLine(x + 1, y + 9, x + 4, y + 12);
    d.drawLine(x + 12, y + 9, x + 9, y + 12);
    d.drawLine(x + 4, y + 12, x + 9, y + 12);
    if (state == Pet::MoodState::VeryHappy) {
        d.drawLine(x + 3, y + 5, x + 4, y + 4);
        d.drawLine(x + 4, y + 4, x + 5, y + 5);
        d.drawLine(x + 8, y + 5, x + 9, y + 4);
        d.drawLine(x + 9, y + 4, x + 10, y + 5);
    } else {
        d.drawLine(x + 4, y + 4, x + 4, y + 5);
        d.drawLine(x + 9, y + 4, x + 9, y + 5);
    }
    switch (state) {
        case Pet::MoodState::VeryHappy:
            d.drawLine(x + 4, y + 8, x + 9, y + 8);
            d.drawLine(x + 4, y + 9, x + 9, y + 9);
            d.drawLine(x + 5, y + 10, x + 8, y + 10);
            break;
        case Pet::MoodState::Happy:
            d.drawLine(x + 4, y + 8, x + 5, y + 9);
            d.drawLine(x + 5, y + 9, x + 8, y + 9);
            d.drawLine(x + 8, y + 9, x + 9, y + 8);
            break;
        case Pet::MoodState::Neutral:
            d.drawLine(x + 4, y + 9, x + 9, y + 9);
            break;
        case Pet::MoodState::Sad:
        case Pet::MoodState::VerySad:
            d.drawLine(x + 4, y + 10, x + 5, y + 9);
            d.drawLine(x + 5, y + 9, x + 8, y + 9);
            d.drawLine(x + 8, y + 9, x + 9, y + 10);
            if (state == Pet::MoodState::VerySad) {
                d.drawLine(x + 3, y + 7, x + 3, y + 9);
                d.drawLine(x + 10, y + 7, x + 10, y + 9);
            }
            break;
    }
}

namespace {
// Shared critical marker, 1 x 5 pixels, kept separate from the main silhouette.
void drawCritical(Hardware::Display& d, int16_t x, int16_t y) {
    d.drawLine(x, y, x, y + 2);
    d.drawLine(x, y + 4, x, y + 4);
}
}  // namespace

void drawHunger(Hardware::Display& d, int16_t x, int16_t y, Pet::HungerState state) {
    // Fixed stomach silhouette: narrow inlet, rounded right pouch, left outlet.
    d.drawLine(x + 4, y, x + 4, y + 4);
    d.drawLine(x + 6, y, x + 6, y + 2);
    d.drawLine(x + 6, y + 2, x + 9, y + 2);
    d.drawLine(x + 9, y + 2, x + 11, y + 4);
    d.drawLine(x + 11, y + 4, x + 11, y + 9);
    d.drawLine(x + 11, y + 9, x + 8, y + 12);
    d.drawLine(x + 8, y + 12, x + 4, y + 12);
    d.drawLine(x + 4, y + 12, x + 2, y + 10);
    d.drawLine(x + 2, y + 10, x, y + 10);
    d.drawLine(x, y + 8, x + 3, y + 8);
    d.drawLine(x + 3, y + 8, x + 5, y + 9);
    d.drawLine(x + 5, y + 9, x + 6, y + 8);
    d.drawLine(x + 6, y + 8, x + 6, y + 6);
    d.drawLine(x + 6, y + 6, x + 4, y + 4);

    int16_t fillTop = 12;
    switch (state) {
        case Pet::HungerState::Satisfied: fillTop = 3; break;
        case Pet::HungerState::SlightlyHungry: fillTop = 6; break;
        case Pet::HungerState::Hungry: fillTop = 9; break;
        case Pet::HungerState::VeryHungry: break;
        case Pet::HungerState::Starving: drawCritical(d, x + 13, y + 1); break;
    }
    // Consecutive scanlines make a solid fill, clipped to the pouch interior.
    const uint8_t left[]  = {7, 5, 6, 7, 7, 7, 6, 4, 5};
    const uint8_t right[] = {9, 10, 10, 10, 10, 10, 10, 9, 8};
    for (int16_t row = fillTop; row <= 11; ++row) {
        d.drawLine(x + left[row - 3], y + row, x + right[row - 3], y + row);
    }
}

void drawCleaningAlert(Hardware::Display& d, int16_t x, int16_t y,
                       Pet::CleanlinessState state) {
    if (state == Pet::CleanlinessState::Clean ||
        state == Pet::CleanlinessState::SlightlyDirty) return;

    // Diagonal handle joins a splayed, striped brush head.
    d.drawLine(x + 7, y, x + 3, y + 5);
    d.drawLine(x + 2, y + 5, x + 5, y + 7);
    d.drawLine(x + 2, y + 5, x, y + 10);
    d.drawLine(x + 3, y + 6, x + 2, y + 11);
    d.drawLine(x + 4, y + 7, x + 4, y + 11);
    d.drawLine(x + 5, y + 7, x + 6, y + 11);
    if (state == Pet::CleanlinessState::Filthy) drawCritical(d, x + 10, y + 2);
}

void drawSick(Hardware::Display& d, int16_t x, int16_t y) {
    // Solid medical cross.
    for (int16_t row = 1; row <= 10; ++row) {
        const bool arm = row >= 4 && row <= 7;
        d.drawLine(x + (arm ? 1 : 4), y + row, x + (arm ? 10 : 7), y + row);
    }
}

void drawStatusCard(Hardware::Display& d, int16_t x, int16_t y) {
    d.drawFrame(x, y, 12, 12);
    for (int16_t row = 3; row <= 9; row += 3) {
        d.drawLine(x + 2, y + row, x + 2, y + row);
        d.drawLine(x + 5, y + row, x + 9, y + row);
    }
}

void drawPetPlaceholderDissolve(Hardware::Display& d, int16_t x, int16_t y,
                                uint8_t stage) {
    // Rounded 44 x 32 slime: soft dome and scalloped base, no limbs or ears.
    const uint8_t outline[][2] = {
        {16, 0}, {27, 0}, {33, 2}, {37, 6}, {40, 12}, {42, 19},
        {43, 25}, {41, 29}, {37, 31}, {30, 31}, {26, 29},
        {23, 31}, {19, 31}, {16, 29}, {12, 31}, {6, 31},
        {2, 29}, {0, 25}, {1, 19}, {3, 12}, {6, 6}, {10, 2}, {16, 0}
    };
    for (unsigned i = 1; i < sizeof(outline) / sizeof(outline[0]); ++i) {
        if (stage == 1 && i % 3 == 1) continue;
        if (stage == 2 && i % 3 != 0) continue;
        if (stage >= 3) continue;
        d.drawLine(x + outline[i - 1][0], y + outline[i - 1][1],
                   x + outline[i][0], y + outline[i][1]);
    }
    if (stage == 0) {
        // Solid bean eyes, small smile and short cheek marks.
        d.drawLine(x + 13, y + 16, x + 13, y + 19);
        d.drawLine(x + 14, y + 16, x + 14, y + 19);
        d.drawLine(x + 29, y + 16, x + 29, y + 19);
        d.drawLine(x + 30, y + 16, x + 30, y + 19);
        d.drawLine(x + 19, y + 21, x + 20, y + 23);
        d.drawLine(x + 20, y + 23, x + 23, y + 23);
        d.drawLine(x + 23, y + 23, x + 24, y + 21);
        d.drawLine(x + 9, y + 22, x + 11, y + 22);
        d.drawLine(x + 32, y + 22, x + 34, y + 22);
    } else if (stage == 1) {
        // The test slime closes its eyes as its outline starts dissolving.
        d.drawLine(x + 12, y + 18, x + 15, y + 18);
        d.drawLine(x + 28, y + 18, x + 31, y + 18);
    } else if (stage == 2) {
        const uint8_t specks[][2] = {
            {8, 12}, {16, 5}, {27, 7}, {36, 14}, {5, 24},
            {17, 27}, {29, 24}, {39, 25},
        };
        for (unsigned i = 0; i < sizeof(specks) / sizeof(specks[0]); ++i) {
            d.drawLine(x + specks[i][0], y + specks[i][1],
                       x + specks[i][0], y + specks[i][1]);
        }
    }
}

void drawPetPlaceholder(Hardware::Display& d, int16_t x, int16_t y) {
    drawPetPlaceholderDissolve(d, x, y, 0);
}

namespace {
// Exact two 30 x 28 frames from assets/ghost/GHOST2.png, top to bottom.
// drawGlyph uses zero bits for white pixels; one bits stay transparent.
const uint8_t kGhostBitmaps[2][112] PROGMEM = {
    {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xC0, 0x0F, 0xFF,
        0xFF, 0xBF, 0xF7, 0xFF,
        0xFF, 0xC0, 0x0F, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xE0, 0x1F, 0xFF,
        0xFF, 0x80, 0x07, 0xFF,
        0xFF, 0x00, 0x03, 0xFF,
        0xFE, 0x00, 0x01, 0xFF,
        0xFE, 0x00, 0x01, 0xFF,
        0xFC, 0x00, 0x00, 0xFF,
        0xF8, 0x44, 0x44, 0x7F,
        0xF8, 0x28, 0x28, 0x7F,
        0x98, 0x10, 0x10, 0x67,
        0x08, 0x28, 0x28, 0x43,
        0xE8, 0x44, 0x44, 0x5F,
        0xF8, 0x00, 0x00, 0x7F,
        0xF8, 0x00, 0x00, 0x7F,
        0xFC, 0x03, 0x80, 0xFF,
        0xFE, 0x04, 0x40, 0xFF,
        0xFE, 0x00, 0x01, 0xFF,
        0xFF, 0x00, 0x03, 0xFF,
        0xFF, 0x80, 0x07, 0xFF,
        0xFF, 0xC0, 0x0F, 0xFF,
        0xFF, 0xC0, 0x3F, 0xFF,
        0xFF, 0x87, 0xFF, 0xFF,
        0xFF, 0x7F, 0xFF, 0xFF,
    },
    {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xC0, 0x0F, 0xFF,
        0xFF, 0xBF, 0xF7, 0xFF,
        0xFF, 0xC0, 0x0F, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xE0, 0x1F, 0xFF,
        0xFF, 0x80, 0x07, 0xFF,
        0xFF, 0x00, 0x03, 0xFF,
        0xFE, 0x00, 0x01, 0xFF,
        0xFE, 0x00, 0x01, 0xFF,
        0xFC, 0x00, 0x00, 0xFF,
        0xF8, 0x44, 0x44, 0x7F,
        0xF8, 0x28, 0x28, 0x7F,
        0xF8, 0x10, 0x10, 0x7F,
        0xE8, 0x28, 0x28, 0x5F,
        0x08, 0x44, 0x44, 0x43,
        0x98, 0x00, 0x00, 0x67,
        0xF8, 0x00, 0x00, 0x7F,
        0xFC, 0x03, 0x80, 0xFF,
        0xFE, 0x04, 0x40, 0xFF,
        0xFE, 0x00, 0x01, 0xFF,
        0xFF, 0x00, 0x03, 0xFF,
        0xFF, 0x80, 0x07, 0xFF,
        0xFF, 0xC0, 0x0F, 0xFF,
        0xFF, 0xC0, 0x3F, 0xFF,
        0xFF, 0x87, 0xFF, 0xFF,
        0xFF, 0x7F, 0xFF, 0xFF,
    },
};
}  // namespace

void drawGhost(Hardware::Display& d, int16_t x, int16_t y, uint8_t frame) {
    d.drawGlyph(x, y, kGhostBitmaps[frame % 2],
                kGhostFrameWidth, kGhostFrameHeight);
}

void drawTombstone(Hardware::Display& d, int16_t x, int16_t y) {
    // 42 x 47: rounded top, straight sides and a slightly wider base.
    d.drawLine(x + 11, y, x + 30, y);
    d.drawLine(x + 6, y + 3, x + 11, y);
    d.drawLine(x + 30, y, x + 35, y + 3);
    d.drawLine(x + 3, y + 7, x + 6, y + 3);
    d.drawLine(x + 35, y + 3, x + 38, y + 7);
    d.drawLine(x + 3, y + 7, x + 3, y + 42);
    d.drawLine(x + 38, y + 7, x + 38, y + 42);
    d.drawLine(x + 3, y + 42, x + 38, y + 42);
    d.drawLine(x, y + 46, x + 41, y + 46);
    d.drawLine(x + 3, y + 42, x, y + 46);
    d.drawLine(x + 38, y + 42, x + 41, y + 46);
}

}  // namespace PetIcons
}  // namespace Ui
