#include <cassert>
#include <cstdio>
#include <cstring>
#include "storage/Adoption.h"
#include "../src/storage/Save.cpp"

FakeNvs nvs;
SerialStub Serial;
uint32_t millis() { return 100; }

int main() {
    using Pet::SpeciesId;
    using Storage::LoadStatus;
    assert(Pet::speciesForNewEgg(0) == SpeciesId::Bird);
    assert(Pet::speciesForNewEgg(1) == SpeciesId::Wyvern);
    assert(Pet::speciesForNewEgg(0xFFFFFFFE) == SpeciesId::Bird);
    assert(Pet::speciesForNewEgg(0xFFFFFFFF) == SpeciesId::Wyvern);
    Storage::Memorials mixed;
    for (auto species : {SpeciesId::Bird, SpeciesId::Wyvern}) {
        nvs = {};
        Pet::PetData pet;
        assert(pet.startNewEgg(static_cast<uint8_t>(species), "Test", species));
        Storage::Save store; assert(store.init() && store.save(pet));
        Pet::PetData recovered;
        Storage::Save reboot; assert(reboot.init());
        assert(reboot.load(recovered) == LoadStatus::Loaded);
        assert(recovered.speciesId() == species && recovered.lifeStage() == Pet::LifeStage::Egg);
        recovered.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
        assert(recovered.speciesId() == species && recovered.lifeStage() == Pet::LifeStage::Baby);
        assert(recovered.beginNormalSleep());
        recovered.advanceSleepSeconds(61);
        assert(recovered.speciesId() == species && recovered.wake());
        auto snapshot = recovered.snapshot();
        snapshot.speciesId = static_cast<SpeciesId>(3);
        assert(!recovered.restore(snapshot));
        assert(!recovered.startNewEgg(7, "Bad", static_cast<SpeciesId>(0)));
        assert(recovered.speciesId() == species);
        recovered.setDead(true);
        assert(mixed.append(recovered));
    }
    nvs = {};
    Storage::Save store; assert(store.init() && store.saveMemorials(mixed));
    Storage::Memorials archive;
    Storage::Save reboot; assert(reboot.init());
    assert(reboot.loadMemorials(archive) == LoadStatus::Loaded && archive.count() == 2);
    assert(archive.at(0)->speciesId == SpeciesId::Bird && archive.at(1)->speciesId == SpeciesId::Wyvern);
    auto invalid = *archive.at(1); invalid.petId = 99; invalid.speciesId = static_cast<SpeciesId>(3);
    assert(!archive.append(invalid));

    // Save failure keeps current pet unchanged and the same candidate on retry.
    for (bool torn : {false, true}) {
        nvs = {};
        Storage::Save save; assert(save.init());
        Pet::PetData current;
        assert(current.startNewEgg(10, "Prior", SpeciesId::Bird));
        current.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
        assert(current.depart() && save.save(current));
        Storage::Adoption adoption;
        assert(adoption.prepare(11, "Dragon", 1));
        nvs.failWrite = nvs.writes + 1; nvs.torn = torn;
        assert(!adoption.commit(save,current));
        assert(adoption.prepared() && current.petId() == 10 && current.isDeparted());
        assert(!adoption.prepare(12, "Reroll", 0));
        nvs.failWrite = 0;
        assert(adoption.commit(save,current) && !adoption.prepared());
        assert(current.petId() == 11 && current.speciesId() == SpeciesId::Wyvern);
        assert(std::strcmp(current.name(),"Dragon") == 0);
        Storage::Save after; assert(after.init());
        Pet::PetData recovered;
        assert(after.load(recovered) == LoadStatus::Loaded && recovered.speciesId() == SpeciesId::Wyvern);
        assert(!adoption.commit(save,current));
    }
    std::puts("PASS: species, hatch/sleep/save, mixed memorials, failed adoption retry without reroll.");
}
