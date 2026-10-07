#include <Arduino.h>
#include <assert.h>
#include <stdio.h>
#include "hardware/Sound.h"

static uint32_t now;
static double frequency;
uint32_t millis() { return now; }
double ledcSetup(uint8_t, double, uint8_t) { return 0; }
void ledcAttachPin(uint8_t, uint8_t) {}
double ledcWriteTone(uint8_t, double hz) { frequency=hz; return hz; }

int main() {
    Hardware::Sound sound;
    sound.init(); assert(!sound.isPlaying() && frequency==0);
    const uint32_t starts[] = {0, UINT32_MAX-100};
    for (uint32_t start : starts) {
        now=start; sound.playCareReminder();
        assert(sound.isPlaying() && frequency==659);
        now+=119; sound.update(); assert(frequency==659);
        ++now; sound.update(); assert(frequency==0 && sound.isPlaying());
        now+=40; sound.update(); assert(frequency==523);
        now+=120; sound.update(); assert(frequency==0);
        now+=40; sound.update(); assert(frequency==392);
        now+=179; sound.update(); assert(frequency==392 && sound.isPlaying());
        ++now; sound.update(); assert(frequency==0 && !sound.isPlaying());
    }
    sound.playCareReminder(); sound.stopTone();
    assert(!sound.isPlaying() && frequency==0);
    puts("PASS: actual non-blocking care reminder frequencies/rests, 500ms duration, stop and clock rollover.");
}
