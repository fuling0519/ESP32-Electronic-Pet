// Web depart/adopt on the real UiController and the real Save codec (fake NVS): the steps of
// Application::handleLifecycleRequest, a silent return from menu and memorial pages, the farewell
// path to the postcard without a confirmation screen, write failures and the kept adoption egg.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main
#include "ble/LifecycleCommand.h"
#include "ble/OpCache.h"
#include "ble/PetEvents.h"
#include "storage/Adoption.h"
#include "storage/Farewell.h"
#include "../src/storage/Save.cpp"

FakeNvs nvs;
uint32_t millis() { return 100; }

using Ui::Activity;
using Ui::ScreenId;
using Ev = Hardware::InputEvent;
static uint32_t t = 0;
static void step(Ui::UiController& ui, Ev e = Ev::None, uint32_t dt = 30) { t += dt; ui.update(e, t); }
static Pet::PetData living(uint64_t id, Pet::LifeStage stage) {
    Pet::PetData pet;
    assert(pet.startNewEgg(id, "Tamama", Pet::SpeciesId::Bird));
    auto snapshot = pet.snapshot(); snapshot.lifeStage = stage;
    snapshot.ageSeconds = stage == Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds :
        stage == Pet::LifeStage::Adult ? Pet::PetData::kAdultAgeSeconds : 0;
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

// The application state handleLifecycleRequest uses, with the same calls in the same order.
struct Device {
    Pet::PetData& pet; Storage::Memorials& memorials; Storage::Save& save; Ui::UiController& ui;
    Storage::Adoption adoption; bool memorialReady = false;
    Device(Pet::PetData& p, Storage::Memorials& m, Storage::Save& s, Ui::UiController& u): pet(p), memorials(m), save(s), ui(u) {}
    const char* activity() const { return Ui::activityName(ui.activity()); }
    // Returns the command_result code; outcome is set when accepted.
    const char* depart(uint64_t petId, Ble::LifecycleOutcome& outcome) {
        const char* code = Ble::departEligibility(petId, pet, activity(), memorials.count());
        if (strcmp(code, "ok") != 0) return code;
        ui.leaveMenuForRemote();
        outcome = Ble::lifecycleFailed("save_failed");
        memorialReady = Storage::beginFarewell(pet, memorials, save);
        ui.setMemorialReady(memorialReady);
        if (pet.isDeparted()) { ui.onFarewellResult(memorialReady, t); outcome = Ble::departDone(memorialReady); }
        return code;
    }
    // adoptNewEgg() with a fixed name instead of the hardware RNG.
    bool adoptNewEgg() {
        if (!pet.isEnded() || !memorialReady || memorials.count() > Storage::kMemorialLimit) return false;
        uint64_t highestId = memorials.highestPetId();
        if (pet.petId() > highestId) highestId = pet.petId();
        if (!adoption.prepared() && !adoption.prepare(highestId + 1, "Kororo", 1)) return false;
        if (!adoption.commit(save, pet)) return false;
        memorialReady = false; ui.setMemorialReady(false); ui.onAdoptionSucceeded(t);
        return true;
    }
    const char* adopt(uint64_t petId, Ble::LifecycleOutcome& outcome) {
        const char* code = Ble::adoptEligibility(petId, pet, activity(), memorials.count(), memorialReady);
        if (strcmp(code, "ok") != 0) return code;
        ui.leaveMenuForRemote();
        outcome = adoptNewEgg() ? Ble::adoptDone(pet) : Ble::lifecycleFailed("save_failed");
        return code;
    }
    // The 10 s retry of the main loop.
    void retryMemorial() { memorialReady = Storage::finishFarewell(pet, memorials, save); ui.setMemorialReady(memorialReady); }
};
static void json(Ble::LifecycleAction a, const Ble::LifecycleOutcome& o, char (&out)[1025]) {
    uint16_t len = 0; assert(Ble::makeLifecycleResultJson(40, 1, a, o, out, sizeof(out), len));
}
// Dies of sickness, saves the death and its memorial like the main loop, and waits for the end screen.
static void die(Device& d) {
    d.pet.setSick(true); d.pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds); assert(d.pet.isDead());
    assert(d.save.save(d.pet));
    d.retryMemorial(); assert(d.memorialReady);
    step(d.ui); step(d.ui, Ev::None, 3700);
    assert(d.ui.screen() == ScreenId::DeathMemorial && d.ui.activity() == Activity::Ended);
}

int main() {
    info.tile_width = 16; info.tile_height = 8; info.pixel_width = 128; info.pixel_height = 64;
    gfx.u8x8.display_info = &info;
    u8g2_SetupBuffer(&gfx, pixels.data(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetDrawColor(&gfx, 1);
    Hardware::Display display;
    char text[1025];
    {
        // Depart from each menu page: back to Home silently, then save, viewfinder and postcard,
        // never through the farewell info or confirmation screens.
        const struct { int item; ScreenId screen; } pages[] = {
            {-1, ScreenId::MainMenu}, {0, ScreenId::FeedCare}, {5, ScreenId::DetailedStatus}, {6, ScreenId::MemorialCategories},
        };
        uint64_t id = 50;
        for (const auto& page : pages) {
            nvs = {};
            Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
            auto pet = living(++id, Pet::LifeStage::Adult); pet.setSick(true);  // Sick pets may be sent off.
            Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
            Device d(pet, records, save, ui);
            if (page.item < 0) step(ui, Ev::Press); else open(ui, static_cast<uint8_t>(page.item));
            assert(ui.screen() == page.screen && ui.activity() == Activity::Menu);
            const unsigned cancels = cancelSounds;
            Ble::LifecycleOutcome o;
            assert(strcmp(d.depart(id, o), "ok") == 0 && cancelSounds == cancels);
            assert(o.done && o.memorialSaved && pet.isDeparted() && records.count() == 1 && records.containsPet(id));
            assert(ui.screen() == ScreenId::FarewellAnimation && ui.activity() == Activity::Busy);
            for (int i = 0; i < 200 && ui.screen() == ScreenId::FarewellAnimation; ++i) {
                assert(ui.screen() != ScreenId::FarewellInfo && ui.screen() != ScreenId::FarewellConfirm);
                assert(ui.takeAction() == Ui::UiAction::None);
                step(ui, Ev::None, 50);
            }
            assert(ui.screen() == ScreenId::FarewellDone && ui.activity() == Activity::Ended);
            json(Ble::LifecycleAction::Depart, o, text);
            assert(strstr(text, R"("action":"depart","outcome":"done","changes":{"memorial_saved":true}})"));
            // The departed flag reached NVS.
            Storage::Save reboot; assert(reboot.init()); Pet::PetData loaded;
            assert(reboot.load(loaded) == Storage::LoadStatus::Loaded && loaded.isDeparted() && loaded.petId() == id);
            // Now ended: a second depart is not allowed; the pet id is checked first.
            assert(strcmp(d.depart(id, o), "not_allowed") == 0 && strcmp(d.depart(id + 100, o), "pet_changed") == 0);
        }
    }
    {
        // Busy pages and a full archive reject the command and leave the screen alone.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        auto pet = living(70, Pet::LifeStage::Baby);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        Ble::LifecycleOutcome o;
        const struct { uint8_t item; ScreenId screen; } busyPages[] = {
            {7, ScreenId::FarewellInfo}, {8, ScreenId::Volume}, {3, ScreenId::GameSelect},
        };
        for (const auto& page : busyPages) {
            home(ui); open(ui, page.item); assert(ui.screen() == page.screen);
            assert(strcmp(d.depart(70, o), "busy") == 0 && ui.screen() == page.screen && !pet.isEnded());
        }
        home(ui); open(ui, 7); step(ui, Ev::Press); assert(ui.screen() == ScreenId::FarewellConfirm);
        assert(strcmp(d.depart(70, o), "busy") == 0 && ui.screen() == ScreenId::FarewellConfirm);
        for (uint64_t i = 1; i <= Storage::kMemorialLimit; ++i) assert(records.append(record(i)));
        home(ui); step(ui, Ev::Press);
        const unsigned writes = nvs.writes;
        assert(strcmp(d.depart(70, o), "memorial_full") == 0 && ui.screen() == ScreenId::MainMenu);
        assert(nvs.writes == writes && !pet.isEnded());
        assert(strcmp(d.depart(71, o), "pet_changed") == 0);
        // An egg and a sleeping pet are not allowed, ahead of memorial_full.
        auto egg = living(72, Pet::LifeStage::Egg); Device e(egg, records, save, ui);
        assert(strcmp(e.depart(72, o), "not_allowed") == 0);
    }
    {
        // Memorial write fails after the departed flag: done with memorial_saved false, the retry
        // page, then the 10 s retry adds it (memorial_added + new hash) and the postcard shows.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        auto pet = living(80, Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        Ble::PetEventDetector events; events.reset(pet, records);
        const uint32_t hashBefore = Storage::memorialHash(records);
        nvs.failWrite = nvs.writes + 4;  // Archive preflight, departed flag, checkpoint, then the append.
        Ble::LifecycleOutcome o;
        assert(strcmp(d.depart(80, o), "ok") == 0);
        nvs.failWrite = 0;
        assert(o.done && !o.memorialSaved && pet.isDeparted() && records.count() == 0 && !d.memorialReady);
        assert(ui.screen() == ScreenId::FarewellBlocked && ui.activity() == Activity::Busy);
        json(Ble::LifecycleAction::Depart, o, text);
        assert(strstr(text, R"("outcome":"done","changes":{"memorial_saved":false}})"));
        Ble::PetEvent ev[8];
        events.markFromWeb(Ble::PetEventType::Departed); events.markFromWeb(Ble::PetEventType::MemorialAdded);
        assert(events.detect(pet, records, ev, 8) == 1 && ev[0].type == Ble::PetEventType::Departed && ev[0].fromWeb);
        // Adoption is not possible while the memorial is unsaved.
        assert(strcmp(d.adopt(80, o), "busy") == 0 && ui.screen() == ScreenId::FarewellBlocked);
        d.retryMemorial();
        assert(d.memorialReady && records.containsPet(80) && Storage::memorialHash(records) != hashBefore);
        uint8_t n = events.detect(pet, records, ev, 8);
        assert(n == 1 && ev[0].type == Ble::PetEventType::MemorialAdded && ev[0].memorialPetId == 80 && !ev[0].fromWeb);
        step(ui); assert(ui.screen() == ScreenId::FarewellDone && ui.activity() == Activity::Ended);
    }
    {
        // The departed flag cannot be saved: failed/save_failed, nothing committed, the device stays
        // on Home (no retry page that would lead to a device confirmation); a new op can try again.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        auto pet = living(90, Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        step(ui, Ev::Right); assert(ui.activity() == Activity::Menu);
        nvs.failWrite = nvs.writes + 2;
        Ble::LifecycleOutcome o;
        assert(strcmp(d.depart(90, o), "ok") == 0);
        nvs.failWrite = 0;
        assert(!o.done && strcmp(o.code, "save_failed") == 0 && !pet.isEnded() && records.count() == 0);
        assert(ui.screen() == ScreenId::Home && ui.activity() == Activity::Idle);
        json(Ble::LifecycleAction::Depart, o, text);
        assert(strstr(text, R"("action":"depart","outcome":"failed","code":"save_failed"})"));
        Storage::Save reboot; assert(reboot.init()); Pet::PetData loaded;
        assert(reboot.load(loaded) != Storage::LoadStatus::Loaded || !loaded.isDeparted());
        assert(strcmp(d.depart(90, o), "ok") == 0 && o.done && o.memorialSaved);
    }
    {
        // Adopt from memorial pages entered from the end screen: back to the end menu silently,
        // then the joystick adoption; the egg is Home's new pet.
        for (int page = 0; page < 3; ++page) {
            nvs = {};
            Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
            assert(records.append(record(3)));
            auto pet = living(100, Pet::LifeStage::Adult);
            Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
            Device d(pet, records, save, ui);
            die(d);
            step(ui, Ev::Press); assert(ui.screen() == ScreenId::DeathOptions);
            step(ui, Ev::Press); assert(ui.screen() == ScreenId::MemorialCategories);
            if (page == 1) { step(ui, Ev::Up); step(ui, Ev::Press); assert(ui.screen() == ScreenId::MemorialHelp); }
            if (page == 2) { step(ui, Ev::Down); step(ui, Ev::Press); assert(ui.screen() == ScreenId::Graveyard); }
            assert(ui.activity() == Activity::Ended);
            // leaveMenuForRemote alone: the end menu, without a sound.
            const unsigned cancels = cancelSounds, confirms = confirmSounds;
            if (page == 0) {
                ui.leaveMenuForRemote();
                assert(ui.screen() == ScreenId::DeathOptions && ui.activity() == Activity::Ended);
                step(ui, Ev::Press); assert(ui.screen() == ScreenId::MemorialCategories);
            }
            Ble::LifecycleOutcome o;
            assert(strcmp(d.adopt(101, o), "pet_changed") == 0 && pet.isDead());
            assert(strcmp(d.adopt(100, o), "ok") == 0 && cancelSounds == cancels);
            assert(page != 0 || confirmSounds == confirms + 1);  // Only the test's own press.
            assert(o.done && o.newPetId == 101 && strcmp(o.name, "Kororo") == 0 && o.speciesId == static_cast<uint8_t>(Pet::SpeciesId::Wyvern));
            assert(pet.petId() == 101 && pet.lifeStage() == Pet::LifeStage::Egg && !pet.isEnded());
            assert(ui.screen() == ScreenId::Home && records.containsPet(100) && !d.memorialReady);
            json(Ble::LifecycleAction::Adopt, o, text);
            assert(strstr(text, R"("action":"adopt","outcome":"done","changes":{"new_pet_id":"101","name":"Kororo","species_id":2}})"));
            // The old target is pet_changed now; a replay is answered by the op cache before this check.
            assert(strcmp(d.adopt(100, o), "pet_changed") == 0);
            Ble::OpCache ops;
            ops.begin(9, Ble::OpKind::Lifecycle, static_cast<uint8_t>(Ble::LifecycleAction::Adopt), 100, 41);
            ops.complete(9, Ble::adoptDone(pet));
            assert(ops.check(9, Ble::OpKind::Lifecycle, static_cast<uint8_t>(Ble::LifecycleAction::Adopt), 100, 42) == Ble::OpCache::Match::Done);
        }
    }
    {
        // A departed pet adopts from the postcard; the overflow slot in use is memorial_full;
        // a failed write keeps the prepared egg and leaves the end menu as it was.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        auto pet = living(200, Pet::LifeStage::Baby);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        Ble::LifecycleOutcome o;
        assert(strcmp(d.depart(200, o), "ok") == 0 && o.done);
        assert(strcmp(d.adopt(200, o), "busy") == 0);  // During the postcard animation.
        step(ui, Ev::None, 3000); assert(ui.screen() == ScreenId::FarewellDone);
        step(ui, Ev::Press); assert(ui.screen() == ScreenId::DeathOptions && ui.activity() == Activity::Ended);
        nvs.failAllWrites = true;
        assert(strcmp(d.adopt(200, o), "ok") == 0);
        assert(!o.done && strcmp(o.code, "save_failed") == 0 && pet.isDeparted() && d.memorialReady);
        assert(ui.screen() == ScreenId::DeathOptions && ui.activity() == Activity::Ended && d.adoption.prepared());
        json(Ble::LifecycleAction::Adopt, o, text);
        assert(strstr(text, R"("action":"adopt","outcome":"failed","code":"save_failed"})"));
        nvs.failAllWrites = false;
        assert(strcmp(d.adopt(200, o), "ok") == 0 && o.done && o.newPetId == 201 && pet.petId() == 201);
    }
    {
        // A death with 32 memorials uses the overflow slot: adopt is memorial_full until one is deleted.
        nvs = {};
        Hardware::Sound sound; Storage::Memorials records; Storage::Save save; assert(save.init());
        for (uint64_t i = 1; i <= Storage::kMemorialLimit; ++i) assert(records.append(record(i)));
        assert(save.saveMemorials(records));
        auto pet = living(300, Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        Device d(pet, records, save, ui);
        die(d);
        assert(records.count() == Storage::kMemorialLimit + 1);
        Ble::LifecycleOutcome o;
        assert(strcmp(d.adopt(300, o), "memorial_full") == 0 && pet.isDead() && ui.screen() == ScreenId::DeathMemorial);
        assert(save.removeMemorial(0, records));
        assert(strcmp(d.adopt(300, o), "ok") == 0 && o.done && o.newPetId == 301);
    }
    puts("PASS: web depart from menu pages to the postcard, unsaved memorial retry, save failures, adopt from end-screen memorial pages, overflow memorial_full.");
}
