// BLE status activity from the real UiController: idle, menu, sleeping, ended, device_game, busy.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main

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
// Home -> main menu -> item, by stepping the menu cursor down.
static void open(Ui::UiController& ui, uint8_t item) {
    if (ui.screen() != Ui::ScreenId::MainMenu) step(ui, Ev::Press);
    assert(ui.screen() == Ui::ScreenId::MainMenu);
    while (ui.menuIndex() != item) step(ui, Ev::Down);
    step(ui, Ev::Press);
}

int main() {
    info.tile_width = 16; info.tile_height = 8; info.pixel_width = 128; info.pixel_height = 64;
    gfx.u8x8.display_info = &info;
    u8g2_SetupBuffer(&gfx, pixels.data(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetDrawColor(&gfx, 1);
    Hardware::Display display;
    assert(strcmp(Ui::activityName(Activity::Idle), "idle") == 0 && strcmp(Ui::activityName(Activity::Menu), "menu") == 0 &&
           strcmp(Ui::activityName(Activity::Sleeping), "sleeping") == 0 && strcmp(Ui::activityName(Activity::Ended), "ended") == 0 &&
           strcmp(Ui::activityName(Activity::DeviceGame), "device_game") == 0 && strcmp(Ui::activityName(Activity::Busy), "busy") == 0);
    {
        // Living pet: menu and status pages are menu; pages with a decision, audition or game are not.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        step(ui, Ev::Right); assert(ui.activity() == Activity::Menu && !ui.webGameStartAllowed());
        step(ui, Ev::Left); assert(ui.activity() == Activity::Idle);
        step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::MainMenu && ui.activity() == Activity::Menu);
        const struct { uint8_t item; Ui::ScreenId screen; Activity activity; } pages[] = {
            {0, Ui::ScreenId::FeedCare, Activity::Menu}, {1, Ui::ScreenId::CleanCare, Activity::Menu},
            {3, Ui::ScreenId::GameSelect, Activity::DeviceGame}, {4, Ui::ScreenId::Rest, Activity::Menu},
            {5, Ui::ScreenId::DetailedStatus, Activity::Menu}, {6, Ui::ScreenId::MemorialCategories, Activity::Menu},
            {7, Ui::ScreenId::FarewellInfo, Activity::Busy}, {8, Ui::ScreenId::Volume, Activity::Busy},
        };
        for (const auto& page : pages) {
            home(ui); open(ui, page.item);
            assert(ui.screen() == page.screen && ui.activity() == page.activity);
        }
        // Memorial help and the record page are menu; the delete confirmation is busy.
        Storage::MemorialRecord old{}; old.petId = 9; strcpy(old.name, "Old"); old.speciesId = Pet::SpeciesId::Bird;
        old.ageSeconds = 60; old.stage = static_cast<uint8_t>(Pet::LifeStage::Adult); assert(records.append(old));
        home(ui); open(ui, 6); step(ui, Ev::Up); step(ui, Ev::Press);
        assert(ui.screen() == Ui::ScreenId::MemorialHelp && ui.activity() == Activity::Menu);
        step(ui, Ev::LongPress); step(ui, Ev::Down); step(ui, Ev::Down); step(ui, Ev::Press);  // Help -> departed -> resting.
        assert(ui.screen() == Ui::ScreenId::Graveyard && ui.activity() == Activity::Menu);
        step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::DeleteMemorialConfirm && ui.activity() == Activity::Busy);
        // A queued action, the care animation and the deep sleep countdown are busy.
        home(ui); open(ui, 0); step(ui, Ev::Press); assert(ui.activity() == Activity::Busy);
        assert(ui.takeAction() == Ui::UiAction::Feed); pet.feed(); ui.onFeedSucceeded(t);
        assert(ui.screen() == Ui::ScreenId::Home && ui.activity() == Activity::Busy);
        home(ui); open(ui, 4); ui.onDeepSleepPending(true); assert(ui.activity() == Activity::Busy);
        ui.onDeepSleepPending(false); assert(ui.activity() == Activity::Menu);
        // The "no treatment needed" notice on the menu is busy until it clears.
        home(ui); open(ui, 2); assert(ui.screen() == Ui::ScreenId::MainMenu && ui.activity() == Activity::Busy);
        step(ui, Ev::None, 1300); assert(ui.activity() == Activity::Menu);
        // Normal sleep: sleeping while quiet, busy once a wake is queued.
        home(ui); open(ui, 4); step(ui, Ev::Press); assert(ui.takeAction() == Ui::UiAction::StartNormalSleep);
        assert(pet.beginNormalSleep()); ui.onSleepStarted(t);
        assert(ui.screen() == Ui::ScreenId::Sleeping && ui.activity() == Activity::Sleeping);
        step(ui, Ev::Press); assert(ui.activity() == Activity::Busy && ui.takeAction() == Ui::UiAction::Wake);
        assert(pet.wake()); ui.onWakeSucceeded(); step(ui); assert(ui.activity() == Activity::Idle);
        // Web play waiting screen is busy here; the application reports web_game for its session.
        ui.beginWebPlay(t); assert(ui.activity() == Activity::Busy);
    }
    {
        // An egg can open the menu.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Egg);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::MainMenu && ui.activity() == Activity::Menu);
    }
    {
        // Death: busy through the animation and until the memorial is saved, then ended on the
        // end screens and memorial pages entered from them; audition and blocked pages are busy.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        pet.setSick(true); pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds); assert(pet.isDead());
        step(ui); assert(ui.screen() == Ui::ScreenId::DeathAnimation && ui.activity() == Activity::Busy);
        step(ui, Ev::None, 3700); assert(ui.screen() == Ui::ScreenId::DeathMemorial && ui.activity() == Activity::Busy);
        ui.setMemorialReady(true); assert(ui.activity() == Activity::Ended);
        step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::DeathOptions && ui.activity() == Activity::Ended);
        step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::MemorialCategories && ui.activity() == Activity::Ended);
        step(ui, Ev::Up); step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::MemorialHelp && ui.activity() == Activity::Ended);
        step(ui, Ev::LongPress); step(ui, Ev::LongPress); assert(ui.screen() == Ui::ScreenId::DeathOptions);
        step(ui, Ev::Up); step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::Volume && ui.activity() == Activity::Busy);
        step(ui, Ev::LongPress); assert(ui.screen() == Ui::ScreenId::DeathOptions && ui.activity() == Activity::Ended);
        ui.setMemorialReady(false); assert(ui.activity() == Activity::Busy);
        step(ui, Ev::Up); step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::AdoptionBlocked && ui.activity() == Activity::Busy);
        ui.setMemorialReady(true); assert(ui.activity() == Activity::Busy);
        step(ui, Ev::LongPress); step(ui, Ev::Press);
        assert(ui.activity() == Activity::Busy && ui.takeAction() == Ui::UiAction::AdoptNewEgg);
    }
    {
        // Farewell: busy through the postcard animation, ended on the done screen.
        Hardware::Sound sound; Storage::Memorials records; auto pet = living(Pet::LifeStage::Baby);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        assert(pet.depart()); ui.setMemorialReady(true); ui.onFarewellResult(true, t);
        assert(ui.screen() == Ui::ScreenId::FarewellAnimation && ui.activity() == Activity::Busy);
        step(ui, Ev::None, 3000); assert(ui.screen() == Ui::ScreenId::FarewellDone && ui.activity() == Activity::Ended);
        step(ui, Ev::Press); assert(ui.screen() == Ui::ScreenId::DeathOptions && ui.activity() == Activity::Ended);
    }
    puts("PASS: BLE activity for idle Home, menu/status/memorial pages, busy decisions and animations, sleeping, ended screens and device games.");
}
