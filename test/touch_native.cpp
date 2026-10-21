#include <Arduino.h>
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include "HardwareConfig.h"
#include "hardware/TouchGesture.h"
#include "hardware/TouchInput.h"

SerialStub Serial;
constexpr uint16_t kTouched = 800;
constexpr uint16_t kReleased = 840;
static_assert(kTouched < HardwareConfig::Touch::TouchBelow &&
              kReleased > HardwareConfig::Touch::ReleaseAbove, "MODE 1 fixtures must span thresholds");
static uint32_t fakeNow;
static uint16_t fakeRaw = kReleased;
static unsigned reads;
static unsigned cycleSets;
uint32_t millis() { return fakeNow; }
uint16_t touchRead(uint8_t pin) { assert(pin == 32); ++reads; return fakeRaw; }
void touchSetCycles(uint16_t measure, uint16_t sleep) {
    assert(reads == 1); // Hardware must be initialized before applying tuning.
    assert(measure == 40000 && sleep == 4096); // Diagnostic MODE 1 contract.
    ++cycleSets;
}
using Hardware::TouchEvent;

struct Trace {
    Hardware::TouchGesture gesture;
    uint32_t origin;
    unsigned pokes = 0, starts = 0, releases = 0;
    explicit Trace(uint32_t offset = 0) : origin(offset) {}
    TouchEvent sample(uint32_t at, uint16_t raw = kTouched, bool allowed = true) {
        return count(gesture.sample(raw, origin + at, allowed));
    }
    TouchEvent tick(uint32_t at, bool allowed = true) {
        return count(gesture.tick(origin + at, allowed));
    }
    TouchEvent count(TouchEvent e) {
        pokes += e == TouchEvent::Poke;
        starts += e == TouchEvent::PettingStarted;
        releases += e == TouchEvent::PettingReleased;
        return e;
    }
    void arm() { sample(0,kReleased); sample(30,kReleased); sample(60,kReleased); }
    void hold(uint32_t from, uint32_t to) {
        for (uint32_t at=from; at<=to; at+=30) sample(at);
    }
};

int main() {
    for (uint32_t origin : {0u, UINT32_MAX-150u}) {
        for (uint32_t duration : {99u,100u,210u,699u}) {
            Trace t(origin); t.arm(); t.hold(100,100+duration-1);
            t.sample(100+duration,kReleased);
            assert(t.tick(100+duration+1) == TouchEvent::None);
            t.sample(130+duration,kReleased); t.sample(160+duration,kReleased);
            assert(t.pokes == (duration>=100 ? 1u : 0u) && !t.starts && !t.releases);
        }
        Trace t(origin); t.arm(); t.hold(100,790);
        assert(t.tick(799) == TouchEvent::None);
        assert(t.tick(800) == TouchEvent::PettingStarted);
        t.hold(820,10000);
        assert(t.starts==1 && !t.pokes);
        // ~90ms dropout is tolerated only after petting starts.
        t.sample(10020,kReleased); t.sample(10050,kReleased); t.sample(10080,kReleased);
        t.sample(10110,kTouched); assert(!t.releases && t.starts==1);
        t.sample(10140,kReleased); t.sample(10170,kReleased); t.sample(10200,kReleased);
        t.sample(10230,kReleased); assert(!t.releases);
        assert(t.sample(10260,kReleased)==TouchEvent::PettingReleased);
        t.sample(10290,kReleased); assert(t.releases==1 && !t.pokes && t.starts==1);
    }
    // Strict threshold equality is a dead band, not a confirmation sample.
    Trace band; band.arm(); band.sample(100); band.sample(130);
    band.sample(160,HardwareConfig::Touch::TouchBelow); band.sample(190); band.sample(220);
    band.tick(221); assert(!band.starts);
    band.sample(250); band.hold(280,850); band.sample(880,HardwareConfig::Touch::ReleaseAbove);
    band.tick(890); assert(!band.starts);
    band.sample(900); assert(band.starts==1);
    // A release beginning at 690ms must not turn into petting at 700ms.
    Trace edge; edge.arm(); edge.hold(100,760); edge.sample(790,kReleased);
    edge.tick(800); edge.sample(820,kReleased); edge.sample(850,kReleased);
    assert(edge.pokes==1 && !edge.starts);
    // Debounce filters single/two-sample spikes; dead band breaks the count.
    Trace noise; noise.arm();
    for(uint32_t at=100;at<1000;at+=90) {
        noise.sample(at,(HardwareConfig::Touch::TouchBelow-1)); noise.sample(at+30,(HardwareConfig::Touch::TouchBelow-1)); noise.sample(at+60,HardwareConfig::Touch::TouchBelow);
    }
    assert(!noise.pokes && !noise.starts);
    // Real separate short touches with 120ms release gaps stay separate.
    Trace repeat; repeat.arm();
    for(uint32_t start=100;start<1000;start+=330) {
        repeat.hold(start,start+180);
        repeat.sample(start+210,kReleased); repeat.sample(start+240,kReleased); repeat.sample(start+270,kReleased);
        repeat.sample(start+300,kReleased);
    }
    assert(repeat.pokes==3 && !repeat.starts);
    // Starting on a forbidden screen, or cancelling an active gesture,
    // requires stable physical release before another event can appear.
    Trace blocked;
    blocked.hold(0,900); blocked.tick(1000); assert(!blocked.starts);
    blocked.arm(); blocked.sample(100,kTouched,false); blocked.hold(130,1000);
    assert(!blocked.starts && !blocked.pokes);
    blocked.sample(1030,kReleased); blocked.sample(1060,kReleased); blocked.sample(1090,kReleased);
    blocked.hold(1120,1810); blocked.tick(1820); assert(blocked.starts==1);
    blocked.gesture.cancel(); blocked.hold(1840,2590); assert(blocked.starts==1);
    // Long sampling stalls and zero readings fail closed, without synthetic samples.
    Trace gap; gap.arm(); gap.hold(100,400);
    assert(gap.sample(1000)==TouchEvent::Cancelled);
    assert(gap.tick(2000)==TouchEvent::Cancelled);
    assert(gap.tick(2001)==TouchEvent::None);
    assert(!gap.starts && !gap.pokes);
    Trace fault; fault.arm(); fault.hold(100,400);
    assert(fault.sample(430,0)==TouchEvent::Cancelled); fault.hold(460,1600);
    assert(!fault.starts && !fault.pokes);
    // Actual sampler reads at most once per interval, with a setup warm-up.
    Hardware::TouchInput input; fakeNow=0; input.init(); assert(reads==1);
    assert(cycleSets==1);
    for(fakeNow=0;fakeNow<90;++fakeNow) assert(input.update(fakeNow,true)==TouchEvent::None);
    assert(reads==3);
    fakeNow=90; input.update(fakeNow,true); assert(reads==4);
    fakeRaw=kTouched;
    for(fakeNow=120;fakeNow<820;++fakeNow) assert(input.update(fakeNow,true)==TouchEvent::None);
    assert(input.update(820,true)==TouchEvent::PettingStarted);
    assert(cycleSets==1); // Runtime sampling must not reconfigure the peripheral.
    input.cancel(); fakeNow=830; assert(input.update(fakeNow,true)==TouchEvent::None);
    puts("PASS: touch thresholds/debounce, 99/100/699/700ms, single long hold, release grace, repeated pokes, cancellation, faults, rollover and real sampler cadence.");
}
