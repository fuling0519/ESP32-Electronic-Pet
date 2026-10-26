// Web care on the real UiController: silent return from menu pages, the remote treatment pour
// settling through the queued Treat action, and an interrupted pour reported as failed.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main
#include "ble/CareCommand.h"

using Ui::Activity;
using Ev = Hardware::InputEvent;
static uint32_t t = 0;
static void step(Ui::UiController& ui, Ev e = Ev::None, uint32_t dt = 30) { t += dt; ui.update(e, t); }
static Pet::PetData living(Pet::LifeStage stage) {
    Pet::PetData pet;
    assert(pet.startNewEgg(321, "Tamama", Pet::SpeciesId::Bird));
    auto snapshot = pet.snapshot(); snapshot.lifeStage = stage;
    snapshot.ageSeconds = stage == Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds :
        stage == Pet::LifeStage::Adult ? Pet::PetData::kAdultAgeSeconds : 0;
    assert(pet.restore(snapshot));
    pet.setMood(80); pet.setSatiety(80); pet.setCleanliness(80);
    return pet;
}
static void home(Ui::UiController& ui) {
    t = 0; ui.init(0); step(ui, Ev::None, 1500); step(ui);
    assert(ui.screen() == Ui::ScreenId::Home && ui.activity() == Activity::Idle);
}
static void open(Ui::UiController& ui, uint8_t item) {
    if (ui.screen() != Ui::ScreenId::MainMenu) step(ui, Ev::Press);
    assert(ui.screen() == Ui::ScreenId::MainMenu);
    while (ui.menuIndex() != item) step(ui, Ev::Down);
    step(ui, Ev::Press);
}
// Runs until the UI queues an action or the time runs out; returns it.
static Ui::UiAction runUntilAction(Ui::UiController& ui, uint32_t maxMs, uint32_t& elapsed) {
    const uint32_t start = t;
    while (t - start < maxMs) {
        step(ui);
        const Ui::UiAction a = ui.takeAction();
        if (a != Ui::UiAction::None) { elapsed = t - start; return a; }
    }
    elapsed = t - start;
    return Ui::UiAction::None;
}

int main() {
    info.tile_width = 16; info.tile_height = 8; info.pixel_width = 128; info.pixel_height = 64;
    gfx.u8x8.display_info = &info;
    u8g2_SetupBuffer(&gfx, pixels.data(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetDrawColor(&gfx, 1);
    Hardware::Display display;
    {
        // Menu pages return to Home silently; other activities stay where they are.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove);
        const struct { uint8_t item; Ui::ScreenId screen; bool menu; } pages[] = {
            {0, Ui::ScreenId::FeedCare, true}, {1, Ui::ScreenId::CleanCare, true},
            {4, Ui::ScreenId::Rest, true}, {5, Ui::ScreenId::DetailedStatus, true},
            {6, Ui::ScreenId::MemorialCategories, true},
            {3, Ui::ScreenId::GameSelect, false}, {7, Ui::ScreenId::FarewellInfo, false},
            {8, Ui::ScreenId::Volume, false},
        };
        for (const auto& page : pages) {
            home(ui); open(ui, page.item); assert(ui.screen() == page.screen);
            const unsigned cancels = cancelSounds, confirms = confirmSounds;
            ui.leaveMenuForRemote();
            assert(cancelSounds == cancels && confirmSounds == confirms);
            if (page.menu) assert(ui.screen() == Ui::ScreenId::Home && ui.activity() == Activity::Idle);
            else assert(ui.screen() == page.screen && ui.activity() != Activity::Idle);
        }
        home(ui); step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::MainMenu);
        unsigned cancels = cancelSounds;
        ui.leaveMenuForRemote(); assert(ui.activity() == Activity::Idle && cancelSounds == cancels);
        // The focused status shortcut on Home counts as menu: focus is cleared.
        home(ui); step(ui, Ev::Right); assert(ui.activity() == Activity::Menu);
        ui.leaveMenuForRemote(); assert(ui.activity() == Activity::Idle && cancelSounds == cancels);
        // Idle Home and the "no treatment needed" notice (busy) are untouched.
        home(ui); ui.leaveMenuForRemote(); assert(ui.activity() == Activity::Idle);
        open(ui, 2); assert(ui.activity() == Activity::Busy);
        ui.leaveMenuForRemote(); assert(ui.screen() == Ui::ScreenId::MainMenu && ui.activity() == Activity::Busy);

        // Remote feed from the feed page: the joystick animation, then the level-up celebration.
        home(ui); open(ui, 0); assert(ui.activity() == Activity::Menu);
        pet.setSatiety(10); pet.setLevel(1); pet.setExp(49);
        ui.leaveMenuForRemote();
        const Pet::PetData before = pet;
        assert(pet.feed()); ui.onFeedSucceeded(t); ui.onCareRewardApplied(before.level(), t);
        const Ble::CareOutcome fed = Ble::careDone(before, pet);
        assert(fed.done && fed.changes.satiety == 20 && fed.changes.exp == 3 && fed.changes.levelsGained == 1);
        assert(ui.screen() == Ui::ScreenId::Home && ui.activity() == Activity::Busy && cancelSounds == cancels);
        for (int i = 0; i < 100; ++i) step(ui, Ev::None, 50);   // 5 s: still celebrating or done.
        for (int i = 0; i < 200 && ui.activity() != Activity::Idle; ++i) step(ui, Ev::None, 50);
        assert(ui.activity() == Activity::Idle && ui.screen() == Ui::ScreenId::Home);
    }
    {
        // Remote treatment from the menu: the pour runs ~1.2 s, then the Treat action settles it.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        pet.setSick(true); step(ui); step(ui, Ev::Press);
        assert(ui.activity() == Activity::Menu);
        ui.leaveMenuForRemote();
        assert(ui.beginRemoteTreatment(t) && ui.treatmentRunning() && ui.activity() == Activity::Busy);
        Ble::PendingTreatment pending; Ble::CareRequest r; r.petId = pet.petId(); r.op = 5; r.action = Ble::CareAction::Treat;
        pending.start(r);
        uint32_t elapsed = 0;
        assert(runUntilAction(ui, 3000, elapsed) == Ui::UiAction::Treat && elapsed >= 1200 && elapsed < 1300);
        Ble::CareOutcome out;
        assert(!pending.interrupted(ui.treatmentRunning(), pet, out));  // Still running until the result.
        const Pet::PetData before = pet;
        assert(pet.treat()); ui.onTreatResult(true, t);
        const Ble::CareOutcome treated = Ble::careDone(before, pet); pending.clear();
        assert(treated.done && treated.changes.exp == 5 && !treated.changes.grew && !ui.treatmentRunning());
        assert(!pending.interrupted(ui.treatmentRunning(), pet, out));
        for (int i = 0; i < 200 && ui.activity() != Activity::Idle; ++i) step(ui, Ev::None, 50);
        assert(ui.activity() == Activity::Idle);
    }
    {
        // Death during the pour: no Treat action, and the pending op fails with not_allowed.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        pet.setSick(true); pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds - 1); assert(!pet.isDead());
        step(ui);
        assert(ui.beginRemoteTreatment(t));
        Ble::PendingTreatment pending; Ble::CareRequest r; r.petId = pet.petId(); r.op = 6; r.action = Ble::CareAction::Treat;
        pending.start(r);
        step(ui, Ev::None, 500);
        pet.advanceSeconds(1); assert(pet.isDead());
        uint32_t elapsed = 0;
        assert(runUntilAction(ui, 2000, elapsed) == Ui::UiAction::None && !ui.treatmentRunning());
        Ble::CareOutcome out;
        assert(pending.interrupted(ui.treatmentRunning(), pet, out) && !out.done && strcmp(out.code, "not_allowed") == 0);
    }
    {
        // A pour that cannot start (healthy pet) reports false; the device shows its notice.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        assert(!ui.beginRemoteTreatment(t) && !ui.treatmentRunning());
    }
    puts("PASS: web care leaves menu pages silently, the remote pour settles through Treat, and an interrupted pour fails.");
}
