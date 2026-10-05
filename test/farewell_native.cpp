#include <cassert>
#include <cstdio>
#include <cstring>
#include "storage/Farewell.h"
// Exercise the actual byte codec as well as public save/recovery APIs.
#include "../src/storage/Save.cpp"

FakeNvs nvs;
SerialStub Serial;
uint32_t millis() { return 100; }
using namespace Storage;

static void repairCrc(std::vector<uint8_t>& data) {
    const auto crc = crc32(data.data(), data.size() - 4);
    for (unsigned i = 0; i < 4; ++i) data[data.size() - 4 + i] = crc >> (i * 8);
}

static std::vector<uint8_t> legacyPet(const Pet::PetData& pet) {
    uint8_t raw[kPetSaveRecordSize];
    assert(encodeRecord(pet.snapshot(), 5, raw));
    std::vector<uint8_t> old(raw, raw + sizeof(raw));
    old.erase(old.begin() + 104); // Remove V2 departure byte.
    old[4] = 1; old[6] = 92;
    repairCrc(old);
    return old;
}

static std::vector<uint8_t> legacyArchive(const Memorials& archive) {
    uint8_t raw[kMemorialRecordSize];
    assert(encodeMemorialRecord(archive, 5, raw));
    std::vector<uint8_t> old(raw, raw + 13);
    for (unsigned i = 0; i < kMemorialCapacity; ++i)
        old.insert(old.end(), raw + 13 + i * kMemorialEntrySize,
                   raw + 13 + i * kMemorialEntrySize + kMemorialEntrySize - 2);
    old.resize(old.size() + 4);
    const unsigned payload = old.size() - 16;
    old[4] = 1; old[6] = payload; old[7] = payload >> 8;
    repairCrc(old);
    return old;
}

int main() {
    // Healthy and sick departures are terminal, distinct from death, no reward.
    for (bool sick : {false, true}) {
        Pet::PetData pet;
        pet.setSick(sick);
        const auto age = pet.ageSeconds();
        const auto exp = pet.exp();
        assert(pet.depart() && pet.isEnded() && !pet.isDead());
        assert(pet.isSick() == sick && !pet.depart());
        assert(!pet.feed() && !pet.clean() && !pet.play() && !pet.treat());
        assert(!pet.beginNormalSleep() && !pet.beginDeepSleep() && !pet.wake());
        pet.advanceSeconds(200000); pet.advanceSleepSeconds(200000);
        pet.setDead(true); pet.setSick(false); pet.gainExp(100);
        assert(pet.ageSeconds() == age && pet.exp() == exp && !pet.isDead());
        assert(Pet::isValidPetSnapshot(pet.snapshot()));
        Pet::PetData restored; assert(restored.restore(pet.snapshot()) && restored.isDeparted());
        auto bad = pet.snapshot(); bad.healthState = Pet::HealthState::Dead;
        assert(!restored.restore(bad));
        bad = pet.snapshot(); bad.sleepMode = Pet::SleepMode::Normal;
        assert(!restored.restore(bad));
    }
    Pet::PetData egg; egg.startNewEgg(); assert(!egg.depart());
    Pet::PetData sleeping; sleeping.beginNormalSleep(); assert(!sleeping.depart());
    Pet::PetData dead; dead.setDead(true); assert(!dead.depart());

    // Upgrade actual V1 byte layouts, then mix V1/V2 A/B slots and reboot.
    nvs = {};
    Pet::PetData pet;
    Memorials old; assert(old.append(dead));
    nvs.bytes["slot_a"] = legacyPet(pet);
    nvs.bytes["mem_a"] = legacyArchive(old);
    Save save; assert(save.init());
    Memorials loaded;
    assert(save.load(pet) == LoadStatus::Loaded && !pet.isDeparted());
    assert(save.loadMemorials(loaded) == LoadStatus::Loaded && loaded.count() == 1);
    assert(loaded.at(0)->kind == FarewellKind::Resting && loaded.at(0)->stage == kUnknownMemorialStage);
    assert(save.save(pet) && save.saveMemorials(loaded));
    Save reboot; reboot.init();
    assert(reboot.load(pet) == LoadStatus::Loaded && reboot.loadMemorials(loaded) == LoadStatus::Loaded);
    assert(nvs.bytes["slot_b"].size() == 109 && nvs.bytes["mem_b"].size() == kMemorialRecordSize);

    // Fault injection at each write boundary, including partially written slots.
    for (bool existing : {false, true}) for (bool torn : {false, true})
    for (unsigned fail = 1; fail <= 4; ++fail) {
        nvs = {};
        Pet::PetData current;
        Memorials archive;
        Save store; store.init();
        assert(store.save(current));
        if (existing) {
            MemorialRecord record{}; record.petId = 25; std::strcpy(record.name, "Prior");
            record.speciesId = Pet::SpeciesId::Bird;
            assert(archive.append(record) && store.saveMemorials(archive));
        }
        nvs.failWrite = nvs.writes + fail; nvs.torn = torn;
        assert(!beginFarewell(current, archive, store));
        nvs.failWrite = 0;
        Save afterReset; afterReset.init();
        Pet::PetData recovered;
        Memorials recoveredArchive;
        assert(afterReset.load(recovered) == LoadStatus::Loaded);
        const auto status = afterReset.loadMemorials(recoveredArchive);
        if (existing) {
            assert(status == LoadStatus::Loaded);
            assert(recoveredArchive.at(0)->petId == 25);
            assert(std::strcmp(recoveredArchive.at(0)->name, "Prior") == 0);
        }
        if (fail <= 2) {
            assert(!recovered.isDeparted());
        } else {
            assert(recovered.isDeparted());
            // A torn first archive write must be recoverable from durable intent.
            assert(status != LoadStatus::Invalid);
            assert(finishFarewell(recovered, recoveredArchive, afterReset));
            assert(finishFarewell(recovered, recoveredArchive, afterReset));
            assert(recoveredArchive.count() == (existing ? 2 : 1));
            assert(recoveredArchive.at(existing ? 1 : 0)->kind == FarewellKind::Departed);
            Pet::PetData next; assert(next.startNewEgg(26, "New"));
            nvs.failWrite = nvs.writes + 1;
            assert(!afterReset.save(next));
            nvs.failWrite = 0;
            Save failedAdoption; failedAdoption.init();
            assert(failedAdoption.load(recovered) == LoadStatus::Loaded && recovered.isDeparted());
            assert(failedAdoption.save(next));
            Save adopted; adopted.init();
            assert(adopted.load(recovered) == LoadStatus::Loaded && recovered.petId() == 26 && !recovered.isDeparted());
        }
    }
    // Shared capacity: voluntary departure reserves overflow for unavoidable death.
    nvs = {};
    Save fullStore; fullStore.init(); Memorials full;
    for (unsigned i = 1; i <= kMemorialLimit; ++i) {
        MemorialRecord record{}; record.petId = i; std::strcpy(record.name, "Old");
        record.speciesId = Pet::SpeciesId::Bird; assert(full.append(record));
    }
    Pet::PetData current; assert(current.startNewEgg(99, "Next"));
    current.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
    assert(!beginFarewell(current, full, fullStore) && !current.isDeparted());
    current.setDead(true); assert(full.append(current) && full.count() == 33);
    // Persistent workspace must not change live records after failed deletion.
    nvs = {};
    Save deletion; deletion.init();
    assert(deletion.saveMemorials(full));
    nvs.failWrite = nvs.writes + 1;
    assert(!deletion.removeMemorial(0, full));
    assert(full.count() == 33 && full.at(0)->petId == 1);
    nvs.failWrite = 0;
    assert(deletion.removeMemorial(0, full));
    assert(full.count() == 32 && full.at(0)->petId == 2);
    Save deletionReboot; deletionReboot.init(); Memorials kept;
    assert(deletionReboot.loadMemorials(kept) == LoadStatus::Loaded);
    assert(kept.count() == 32 && kept.at(0)->petId == 2);
    puts("PASS: farewell domain, V1 migration, restart/fault injection, adoption and capacity.");
}
