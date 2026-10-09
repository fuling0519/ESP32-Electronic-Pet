#include <cassert>
#include <cstdio>
#include "storage/Save.h"
#include "../src/storage/Save.cpp"

FakeNvs nvs;
SerialStub Serial;
uint32_t clockMs = 100;
uint32_t millis() { return clockMs; }

static Pet::PetData readyPet() {
    Pet::PetData p;
    assert(p.startNewEgg(1, "Retry", Pet::SpeciesId::Wyvern));
    p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
    return p;
}

int main() {
    auto p = readyPet();
    Storage::Save save;
    assert(save.init() && !save.capacityLow() && save.save(p));
    p.setSatiety(70); assert(p.feed()); save.markDirty(200);
    nvs.failAllWrites = true; nvs.freeEntries = 14;
    const unsigned before = nvs.writes;
    clockMs = 10100; save.update(p, clockMs);
    assert(nvs.writes == before+1 && save.hasSaveFailure() && save.capacityLow());
    assert(save.retryDelayMs() == 5000);
    for (clockMs = 10101; clockMs < 15100; ++clockMs) save.update(p,clockMs);
    assert(nvs.writes == before+1);
    save.update(p,clockMs); assert(nvs.writes == before+2 && save.retryDelayMs() == 10000);
    for (clockMs = 15101; clockMs < 25100; ++clockMs) save.update(p,clockMs);
    assert(nvs.writes == before+2);
    save.update(p,clockMs); assert(nvs.writes == before+3 && save.retryDelayMs() == 30000);
    for (clockMs = 25101; clockMs < 55100; ++clockMs) save.update(p,clockMs);
    assert(nvs.writes == before+3);
    save.update(p,clockMs); assert(nvs.writes == before+4 && save.retryDelayMs() == 60000);
    for (clockMs = 55101; clockMs < 115100; ++clockMs) save.update(p,clockMs);
    assert(nvs.writes == before+4);
    save.update(p,clockMs); assert(nvs.writes == before+5 && save.retryDelayMs() == 60000);
    nvs.failAllWrites = false; nvs.freeEntries = 400;
    p.setMood(91); // Changes made during the outage must survive recovery.
    clockMs = 175100; save.update(p,clockMs);
    assert(nvs.writes == before+6 && !save.hasSaveFailure() && !save.capacityLow());
    assert(save.retryDelayMs() == 0);
    Storage::Save reboot; assert(reboot.init()); Pet::PetData recovered;
    assert(reboot.load(recovered) == Storage::LoadStatus::Loaded && recovered.mood() == 91);
    const auto savedWrites = nvs.writes;
    save.update(p, clockMs-1); assert(nvs.writes == savedWrites); // Old loop timestamp.
    nvs.freeEntries = 159; save.refreshCapacity(); assert(save.capacityLow());
    nvs.freeEntries = 160; save.refreshCapacity(); assert(!save.capacityLow());
    nvs.failStats = true; save.refreshCapacity(); assert(!save.capacityLow());
    nvs.failStats = false;

    // Backoff must survive the uint32_t millis rollover.
    clockMs = 0xffff0000; nvs = {}; Storage::Save rollover;
    assert(rollover.init() && rollover.save(p));
    clockMs = 0xfffffff0; rollover.markDirty(clockMs-2000);
    nvs.failAllWrites = true; rollover.update(p,clockMs);
    const auto failedAt = clockMs; const auto failedWrites = nvs.writes;
    clockMs = failedAt+4999; rollover.update(p,clockMs); assert(nvs.writes == failedWrites);
    clockMs = failedAt+5000; rollover.update(p,clockMs); assert(nvs.writes == failedWrites+1);
    // Explicit checkpoints may be retried by the player immediately.
    nvs.failAllWrites = false; assert(rollover.save(p) && !rollover.hasSaveFailure());
    puts("PASS: persistent NVS failure backoff 5/10/30/60s, pending changes, recovery, capacity reserve, stale timestamps and rollover.");
}
