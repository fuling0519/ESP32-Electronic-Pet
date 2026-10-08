#include <Arduino.h>
#include <assert.h>
#include <stdio.h>
#include "hardware/Sound.h"

static uint32_t now;
static double frequency;
static uint32_t duty;
static uint8_t resolution;
uint32_t millis() { return now; }
double ledcSetup(uint8_t, double hz, uint8_t bits) { frequency=hz; resolution=bits; return hz; }
void ledcAttachPin(uint8_t, uint8_t) {}
double ledcWriteTone(uint8_t, double hz) { frequency=hz; if(!hz) duty=0; return hz; }
void ledcWrite(uint8_t, uint32_t value) { duty=value; if(!value) frequency=0; }

int main() {
    Hardware::Sound sound;
    sound.init(); assert(!sound.isPlaying() && frequency==0);
    const uint32_t duties[] = {0,128};
    for(uint8_t level=0;level<2;++level) {
        sound.setVolume(level);
        now=0; sound.playTone(523,120);
        assert(duty==duties[level] && sound.isPlaying());
        if(level) assert(frequency==523 && resolution==8);
        now=119; sound.update(); assert(sound.isPlaying());
        now=120; sound.update(); assert(!sound.isPlaying() && duty==0);
        now=0; sound.playSuccess(); assert(duty==duties[level]);
        now=80; sound.update(); assert(duty==duties[level] && sound.isPlaying());
        now=200; sound.update(); assert(!sound.isPlaying() && duty==0);
    }
    sound.setVolume(255); assert(sound.volume()==1);
    now=0; sound.playTone(659,120);
    now=50; sound.setVolume(0); assert(duty==0 && sound.isPlaying());
    now=80; sound.setVolume(1); assert(duty==128 && frequency==659);
    now=120; sound.update(); assert(!sound.isPlaying());
    sound.setVolume(1);
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
    for(uint8_t mode=0;mode<2;++mode) {
        sound.setVolume(mode);
        now=0; sound.playConfirm();
        assert(sound.isPlaying() && duty==(mode?128u:0u));
        if(mode) assert(frequency==1047);
        now=70; sound.update(); assert(!sound.isPlaying() && duty==0);
        now=0; sound.playCancel();
        assert(sound.isPlaying() && duty==(mode?128u:0u));
        if(mode) assert(frequency==392);
        now=100; sound.update(); assert(!sound.isPlaying() && duty==0);
    }
    puts("PASS: actual non-blocking care reminder frequencies/rests, 500ms duration, stop and clock rollover.");
}
