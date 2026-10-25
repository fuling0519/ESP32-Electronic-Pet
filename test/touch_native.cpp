#include <Arduino.h>
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include "HardwareConfig.h"
#include "hardware/TouchGesture.h"
#include "hardware/TouchInput.h"

SerialStub Serial;
constexpr uint16_t kTouched = 620;
constexpr uint16_t kReleased = 730;
// Thresholds once the baseline has learned kReleased.
constexpr uint16_t kTouchBelow = kReleased - HardwareConfig::Touch::TouchDrop;
constexpr uint16_t kReleaseAbove = kReleased - HardwareConfig::Touch::ReleaseDrop;
static_assert(kTouched < kTouchBelow, "MODE 1 fixtures must span thresholds");
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
    // Sweep the assembled-board range extremes at the production read cadence.
    // These test amplitudes use reported values, not captured hardware timings.
    const uint16_t releasedRange[] = {718, 725, 737, 745};
    const uint16_t touchedRange[] = {592, 603, 619, 632, 642};
    Trace assembled;
    for (uint32_t i=0; i<30; ++i) assembled.sample(i*30,releasedRange[i%4]);
    assert(!assembled.pokes && !assembled.starts && !assembled.releases);
    for (uint32_t i=0; i<35; ++i) assembled.sample(900+i*30,touchedRange[i%5]);
    assert(assembled.starts==1 && !assembled.pokes && !assembled.releases);
    for (uint32_t i=0; i<10; ++i) assembled.sample(1950+i*30,releasedRange[i%4]);
    assert(assembled.starts==1 && assembled.releases==1 && !assembled.pokes);
    Trace middle; middle.arm();
    for (uint32_t at=100; at<1000; at+=30) middle.sample(at,kReleased-16);
    assert(!middle.pokes && !middle.starts && !middle.releases);
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
    band.sample(160,kTouchBelow); band.sample(190); band.sample(220);
    band.tick(221); assert(!band.starts);
    band.sample(250); band.hold(280,850); band.sample(880,kReleaseAbove);
    band.tick(890); assert(!band.starts);
    band.sample(900); assert(band.starts==1);
    // A release beginning at 690ms must not turn into petting at 700ms.
    Trace edge; edge.arm(); edge.hold(100,760); edge.sample(790,kReleased);
    edge.tick(800); edge.sample(820,kReleased); edge.sample(850,kReleased);
    assert(edge.pokes==1 && !edge.starts);
    // Debounce filters single/two-sample spikes; dead band breaks the count.
    Trace noise; noise.arm();
    for(uint32_t at=100;at<1000;at+=90) {
        const uint16_t below = noise.gesture.baseline() - HardwareConfig::Touch::TouchDrop;
        noise.sample(at,below-1); noise.sample(at+30,below-1); noise.sample(at+60,below);
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
    // Touched at boot: the baseline starts low and climbs back within ~1.5 s of release.
    blocked.hold(0,900); blocked.tick(1000); assert(!blocked.starts);
    for (uint32_t at=1030; at<=2500; at+=30) blocked.sample(at,kReleased);
    assert(blocked.gesture.baseline() >= kReleased-4 && !blocked.starts && !blocked.pokes);
    blocked.sample(2530,kTouched,false); blocked.hold(2560,3430);
    assert(!blocked.starts && !blocked.pokes);
    blocked.sample(3460,kReleased); blocked.sample(3490,kReleased); blocked.sample(3520,kReleased);
    blocked.hold(3550,4240); blocked.tick(4250); assert(blocked.starts==1);
    blocked.gesture.cancel(); blocked.hold(4270,5020); assert(blocked.starts==1);
    // Long sampling stalls and zero readings fail closed, without synthetic samples.
    Trace gap; gap.arm(); gap.hold(100,400);
    assert(gap.sample(1000)==TouchEvent::Cancelled);
    assert(gap.tick(2000)==TouchEvent::Cancelled);
    assert(gap.tick(2001)==TouchEvent::None);
    assert(!gap.starts && !gap.pokes);
    Trace fault; fault.arm(); fault.hold(100,400);
    assert(fault.sample(430,0)==TouchEvent::Cancelled); fault.hold(460,1600);
    assert(!fault.starts && !fault.pokes);
    // 2026-10-25 MODE 1 recording: released 640-648, touched 573-612 after a
    // 538-646 contact transient. The old fixed 670/690 thresholds never armed.
    const uint16_t todayReleased[] = {641,647,645,647,646,646,645,644,643,642,642,641,640,640,643,648};
    const uint16_t todayTouched[] = {538,560,569,646,646,646,641,615,596,594,592,589,587,584,581,579,577,
        576,574,573,575,578,577,580,584,590,595,601,602,599,592,585,578,576,581,574,584,590,597,600,
        601,598,592,584,577,574,576,579,585,593,603,610,612,608,585,606,582,603,579,583,590,597,601,605};
    Trace today; uint32_t at=0;
    for (int round=0; round<4; ++round) for (uint16_t raw : todayReleased) { today.sample(at,raw); at+=30; }
    assert(today.gesture.baseline() >= 640 && today.gesture.baseline() <= 648);
    assert(!today.pokes && !today.starts && !today.releases);
    for (uint16_t raw : todayTouched) { today.sample(at,raw); at+=30; }
    assert(today.starts==1 && !today.pokes && !today.releases);
    for (uint16_t raw : todayReleased) { today.sample(at,raw); at+=30; }
    assert(today.starts==1 && today.releases==1 && !today.pokes);
    // A short poke on the same board.
    for (int i=0; i<8; ++i) { today.sample(at,i<2 ? 600+i*10 : 585); at+=30; }
    for (uint16_t raw : todayReleased) { today.sample(at,raw); at+=30; }
    assert(today.pokes==1 && today.starts==1 && today.releases==1);
    // Slow drift from the 10-22 level to the 10-25 level is followed without events.
    Trace drift; drift.arm();
    for (uint32_t i=0; i<=2000; ++i) drift.sample(90+i*30, static_cast<uint16_t>(730 - (86*i)/2000 + (i%3)));
    assert(drift.gesture.baseline() >= 642 && drift.gesture.baseline() <= 648);
    assert(!drift.pokes && !drift.starts && !drift.releases);
    drift.hold(60200,61000); assert(drift.starts==1);
    // A sudden step down reads as one long touch; after RebaselineMs it is
    // cancelled, the baseline restarts there, and real touches work again.
    Trace step; step.arm();
    uint32_t t=90, stepAt=t;
    TouchEvent last=TouchEvent::None;
    for (; t<stepAt+HardwareConfig::Touch::RebaselineMs+60; t+=30) {
        last=step.sample(t,644);
        if (last==TouchEvent::Cancelled) break;
    }
    assert(last==TouchEvent::Cancelled && step.starts==1 && step.gesture.baseline()==644);
    for (int i=0; i<5; ++i) step.sample(t+=30,644);
    const unsigned startsBefore=step.starts;
    for (uint32_t end=t+800; t<end;) step.sample(t+=30,590);
    assert(step.starts==startsBefore+1);
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
    puts("PASS: learned baseline (10-25 recording, slow drift, step rebaseline, touched at boot), touch thresholds/debounce, 99/100/699/700ms, single long hold, release grace, repeated pokes, cancellation, faults, rollover and real sampler cadence.");
}
