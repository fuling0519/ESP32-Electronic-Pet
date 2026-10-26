// Web delete_memorial on the real UiController and the real Save codec (fake NVS): the steps of
// Application::handleMemorialRequest, a silent return from memorial pages (living pet to Home,
// ended pet to the end menu), busy on the delete confirmation, sleeping, protection, save failure,
// the persisted archive, the web event source and an op replay after the deletion.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main
#include "ble/MemorialCommand.h"
#include "ble/OpCache.h"
#include "ble/PetEvents.h"
#include "storage/Farewell.h"
#include "../src/storage/Save.cpp"

FakeNvs nvs;
uint32_t millis() { return 100; }

using Ui::Activity;
using Ui::ScreenId;
using Ev = Hardware::InputEvent;
static uint32_t t = 0;
static void step(Ui::UiController& ui, Ev e = Ev::None, uint32_t dt = 30) { t += dt; ui.update(e, t); }
static Pet::PetData living(uint64_t id) {
    Pet::PetData pet;
    assert(pet.startNewEgg(id, "Tamama", Pet::SpeciesId::Bird));
    auto snapshot = pet.snapshot(); snapshot.lifeStage = Pet::LifeStage::Adult;
    snapshot.ageSeconds = Pet::PetData::kAdultAgeSeconds;
    assert(pet.restore(snapshot));
    pet.setMood(80); pet.setSatiety(80); pet.setCleanliness(80);
    return pet;
}
static Storage::MemorialRecord record(uint64_t id) {
    Storage::MemorialRecord r{}; r.petId = id; strcpy(r.name, "Old"); r.speciesId = Pet::SpeciesId::Bird;
    r.ageSeconds = 60; r.stage = static_cast<uint8_t>(Pet::LifeStage::Adult);
    return r;
}
static void home(Ui::UiController& ui) {
    t = 0; ui.init(0); step(ui, Ev::None, 1500); step(ui);
    assert(ui.screen() == ScreenId::Home && ui.activity() == Activity::Idle);
}
static void open(Ui::UiController& ui, uint8_t item) {
    if (ui.screen() != ScreenId::MainMenu) step(ui, Ev::Press);
    assert(ui.screen() == ScreenId::MainMenu);
    while (ui.menuIndex() != item) step(ui, Ev::Down);
    step(ui, Ev::Press);
}
// From the memorial categories: 0 categories, 1 help, 2 resting records, 3 delete confirmation.
static void memorialPage(Ui::UiController& ui, int page) {
    assert(ui.screen() == ScreenId::MemorialCategories);
    if (page == 1) { step(ui, Ev::Up); step(ui, Ev::Press); assert(ui.screen() == ScreenId::MemorialHelp); }
    if (page >= 2) { step(ui, Ev::Down); step(ui, Ev::Press); assert(ui.screen() == ScreenId::Graveyard); }
    if (page == 3) { step(ui, Ev::Press); assert(ui.screen() == ScreenId::DeleteMemorialConfirm); }
}

// The application state handleMemorialRequest uses, with the same calls in the same order, plus the
// Link's op cache in front of it.
struct Device {
    Pet::PetData& pet; Storage::Memorials& memorials; Storage::Save& save; Ui::UiController& ui;
    Ble::PetEventDetector events; Ble::OpCache ops;
    Device(Pet::PetData& p, Storage::Memorials& m, Storage::Save& s, Ui::UiController& u): pet(p), memorials(m), save(s), ui(u) {
        events.reset(p, m);
    }
    // The command_result code; outcome is set when accepted or replayed.
    const char* remove(uint64_t memorialPetId, uint64_t op, Ble::MemorialOutcome& outcome) {
        switch (ops.check(op, Ble::OpKind::Memorial, 0, memorialPetId, 1)) {
            case Ble::OpCache::Match::Conflict: return "op_conflict";
            case Ble::OpCache::Match::Pending: return "ok";
            case Ble::OpCache::Match::Done: outcome = ops.find(op)->memorial; return "ok";
            case Ble::OpCache::Match::New: break;
        }
        const char* code = Ble::deleteMemorialEligibility(memorialPetId, pet, memorials, Ui::activityName(ui.activity()));
        if (strcmp(code, "ok") != 0) return code;
        ops.begin(op, Ble::OpKind::Memorial, 0, memorialPetId, 1);
        ui.leaveMenuForRemote();
        const int index = Ble::findMemorial(memorials, memorialPetId);
        const bool removed = index >= 0 && save.removeMemorial(static_cast<uint8_t>(index), memorials);
        if (removed) events.markFromWeb(Ble::PetEventType::MemorialDeleted);
        outcome = removed ? Ble::memorialDone() : Ble::memorialFailed("save_failed");
        ops.complete(op, outcome);
        return code;
    }
};
static bool persisted(uint64_t id) {
    Storage::Save reboot; assert(reboot.init()); Storage::Memorials loaded;
    assert(reboot.loadMemorials(loaded) == Storage::LoadStatus::Loaded);
    return loaded.containsPet(id);
}
// Dies of sickness, saves the death and its memorial like the main loop, and waits for the end screen.
static void die(Device& d) {
    d.pet.setSick(true); d.pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds); assert(d.pet.isDead());
    assert(d.save.save(d.pet));
    const bool ready = Storage::finishFarewell(d.pet, d.memorials, d.save); assert(ready);
    d.ui.setMemorialReady(ready);
    step(d.ui); step(d.ui, Ev::None, 3700);
    assert(d.ui.screen() == ScreenId::DeathMemorial && d.ui.activity() == Activity::Ended);
    d.events.reset(d.pet, d.memorials);
}

int main() {
    info.tile_width = 16; info.tile_height = 8; info.pixel_width = 128; info.pixel_height = 64;
    gfx.u8x8.display_info = &info;
    u8g2_SetupBuffer(&gfx, pixels.data(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetDrawColor(&gfx, 1);
    Hardware::Display display;
    {
        // A living pet on each memorial page: back to Home silently, the record leaves NVS, the
        // event says web; the delete confirmation page is busy and stays.
        for (int page = 0; page < 4; ++page) {
            nvs = {};
            Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
            for (uint64_t i = 1; i <= 3; ++i) assert(records.append(record(i)));
            assert(save.saveMemorials(records));
            auto pet = living(10);
            Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
            Device d(pet, records, save, ui);
            open(ui, 6); memorialPage(ui, page);
            const ScreenId screen = ui.screen();
            const uint32_t hashBefore = Storage::memorialHash(records);
            Ble::MemorialOutcome o;
            if (page == 3) {
                assert(ui.activity() == Activity::Busy);
                const unsigned writes = nvs.writes;
                assert(strcmp(d.remove(2, 0x10, o), "busy") == 0);
                assert(ui.screen() == ScreenId::DeleteMemorialConfirm && records.count() == 3 && nvs.writes == writes);
                // Rejected ops are not recorded: after leaving the page the same op is checked again.
                step(ui, Ev::LongPress); assert(ui.screen() == ScreenId::Graveyard);
            }
            assert(ui.activity() == Activity::Menu);
            const unsigned cancels = cancelSounds;
            assert(strcmp(d.remove(2, 0x10, o), "ok") == 0 && o.done && cancelSounds == cancels);
            assert(ui.screen() == ScreenId::Home && ui.activity() == Activity::Idle && screen != ScreenId::Home);
            assert(records.count() == 2 && !records.containsPet(2) && !persisted(2) && persisted(1) && persisted(3));
            assert(Storage::memorialHash(records) != hashBefore);
            Ble::PetEvent ev[8];
            assert(d.events.detect(pet, records, ev, 8) == 1 && ev[0].type == Ble::PetEventType::MemorialDeleted &&
                   ev[0].memorialPetId == 2 && ev[0].fromWeb);
            // A replay after the deletion: the stored result, not not_found; a new op is not_found.
            const unsigned writes = nvs.writes;
            Ble::MemorialOutcome replay;
            assert(strcmp(d.remove(2, 0x10, replay), "ok") == 0 && replay.done && nvs.writes == writes);
            assert(strcmp(d.remove(2, 0x11, replay), "not_found") == 0);
            assert(strcmp(d.remove(3, 0x10, replay), "op_conflict") == 0);
            // Entering the records again starts from a valid record.
            open(ui, 6); memorialPage(ui, 2);
            assert(records.at(ui.selectedMemorialIndex()) != nullptr);
        }
    }
    {
        // Home, a menu page and normal sleep are accepted; sleep stays on its screen. Games and
        // the volume page are busy.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        for (uint64_t i = 1; i <= 5; ++i) assert(records.append(record(i)));
        auto pet = living(20);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        Ble::MemorialOutcome o;
        assert(strcmp(d.remove(1, 1, o), "ok") == 0 && o.done && ui.screen() == ScreenId::Home);
        open(ui, 5); assert(ui.screen() == ScreenId::DetailedStatus);
        assert(strcmp(d.remove(2, 2, o), "ok") == 0 && o.done && ui.screen() == ScreenId::Home);
        const struct { uint8_t item; ScreenId screen; } busyPages[] = { {8, ScreenId::Volume}, {3, ScreenId::GameSelect} };
        for (const auto& page : busyPages) {
            home(ui); open(ui, page.item); assert(ui.screen() == page.screen);
            assert(strcmp(d.remove(3, 3, o), "busy") == 0 && ui.screen() == page.screen && records.containsPet(3));
        }
        home(ui);
        assert(pet.beginNormalSleep()); ui.onSleepStarted(t);
        assert(ui.screen() == ScreenId::Sleeping && ui.activity() == Activity::Sleeping);
        assert(strcmp(d.remove(3, 3, o), "ok") == 0 && o.done && !records.containsPet(3));
        assert(ui.screen() == ScreenId::Sleeping && pet.sleepMode() == Pet::SleepMode::Normal);
    }
    {
        // An ended pet: memorial pages go back to the end menu; its own record is protected
        // everywhere, and not_found comes first.
        for (int page = 0; page < 4; ++page) {
            nvs = {};
            Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
            assert(records.append(record(3))); assert(records.append(record(4)));
            auto pet = living(100);
            Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
            Device d(pet, records, save, ui);
            die(d);
            assert(records.count() == 3 && records.containsPet(100));
            Ble::MemorialOutcome o;
            assert(strcmp(d.remove(100, 0x20, o), "not_allowed") == 0 && ui.screen() == ScreenId::DeathMemorial);
            step(ui, Ev::Press); assert(ui.screen() == ScreenId::DeathOptions);
            step(ui, Ev::Press); memorialPage(ui, page);
            if (page == 3) {
                assert(ui.activity() == Activity::Busy && strcmp(d.remove(3, 0x21, o), "busy") == 0);
                assert(ui.screen() == ScreenId::DeleteMemorialConfirm && records.containsPet(3));
                step(ui, Ev::LongPress);
            }
            assert(ui.activity() == Activity::Ended);
            assert(strcmp(d.remove(100, 0x20, o), "not_allowed") == 0 && ui.screen() != ScreenId::DeathOptions);
            assert(strcmp(d.remove(99, 0x20, o), "not_found") == 0);
            const unsigned cancels = cancelSounds;
            assert(strcmp(d.remove(3, 0x21, o), "ok") == 0 && o.done && cancelSounds == cancels);
            assert(ui.screen() == ScreenId::DeathOptions && ui.activity() == Activity::Ended);
            assert(!records.containsPet(3) && records.containsPet(100) && !persisted(3) && persisted(100));
            // From the end menu itself: stays there.
            assert(strcmp(d.remove(4, 0x22, o), "ok") == 0 && o.done && ui.screen() == ScreenId::DeathOptions);
            assert(records.count() == 1);
        }
    }
    {
        // A write failure: failed/save_failed, the record stays (RAM and NVS); the page was left.
        // The failed result replays; a new op succeeds once writes work again.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        for (uint64_t i = 1; i <= 3; ++i) assert(records.append(record(i)));
        assert(save.saveMemorials(records));
        auto pet = living(30);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        open(ui, 6); memorialPage(ui, 2);
        const uint32_t hashBefore = Storage::memorialHash(records);
        nvs.failAllWrites = true;
        Ble::MemorialOutcome o;
        assert(strcmp(d.remove(1, 0x30, o), "ok") == 0 && !o.done && strcmp(o.code, "save_failed") == 0);
        nvs.failAllWrites = false;
        assert(records.count() == 3 && records.containsPet(1) && persisted(1) && Storage::memorialHash(records) == hashBefore);
        assert(ui.screen() == ScreenId::Home);
        Ble::PetEvent ev[8]; assert(d.events.detect(pet, records, ev, 8) == 0);
        char json[1025]; uint16_t len = 0;
        assert(Ble::makeMemorialResultJson(31, 0x30, o, json, sizeof(json), len));
        assert(strstr(json, R"("action":"delete_memorial","outcome":"failed","code":"save_failed"})"));
        Ble::MemorialOutcome replay;
        assert(strcmp(d.remove(1, 0x30, replay), "ok") == 0 && !replay.done && strcmp(replay.code, "save_failed") == 0);
        assert(records.containsPet(1));
        assert(strcmp(d.remove(1, 0x31, o), "ok") == 0 && o.done && !records.containsPet(1) && !persisted(1));
    }
    puts("PASS: web delete_memorial from memorial pages (Home or end menu), busy on the confirmation page, sleeping, protection, save failure, NVS, web event and replay.");
}
