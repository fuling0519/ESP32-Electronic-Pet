// The device's "web play" waiting screen with the real UiController, PetIcons and U8g2 framebuffer.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main

static Pet::PetData living(Pet::SpeciesId species, Pet::LifeStage stage) {
    Pet::PetData pet;
    assert(pet.startNewEgg(321, "Tamama", species));
    auto snapshot = pet.snapshot(); snapshot.lifeStage = stage;
    snapshot.ageSeconds = stage == Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds : Pet::PetData::kAdultAgeSeconds;
    assert(pet.restore(snapshot)); return pet;
}
static void home(Ui::UiController& ui) {
    ui.init(0); ui.update(Hardware::InputEvent::None, 1500); ui.update(Hardware::InputEvent::None, 2000); ui.render();
    assert(ui.screen() == Ui::ScreenId::Home);
}
// Any lit pixel in the text row below the pet (y 51..62).
static bool textRowLit() {
    for (unsigned y = 51; y < 63; ++y) for (unsigned x = 20; x < 108; ++x) if (lit(x, y)) return true;
    return false;
}

int main() {
    info.tile_width = 16; info.tile_height = 8; info.pixel_width = 128; info.pixel_height = 64;
    gfx.u8x8.display_info = &info;
    u8g2_SetupBuffer(&gfx, pixels.data(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetDrawColor(&gfx, 1);
    Hardware::Display display; Storage::Memorials records;
    const char* keys[] = {"bird-baby", "bird-adult", "wyvern-baby", "wyvern-adult"};
    for (unsigned role = 0; role < 4; ++role) {
        Hardware::Sound sound;
        auto pet = living(role < 2 ? Pet::SpeciesId::Bird : Pet::SpeciesId::Wyvern,
                          role % 2 ? Pet::LifeStage::Adult : Pet::LifeStage::Baby);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        assert(ui.webGameStartAllowed() && !ui.deviceGameActive());
        ui.beginWebPlay(2000); ui.render();
        assert(ui.screen() == Ui::ScreenId::WebPlay && strcmp(ui.screenName(), "WEB_PLAY") == 0);
        assert(!ui.webGameStartAllowed() && !ui.touchContextAllowed());
        assert(textRowLit());
        char file[100]; snprintf(file, sizeof(file), "web-play-%s-playing", keys[role]); capture(ui, file);
        const auto playing = pixels;
        // Ordinary input is ignored; after 2 s the line switches to the long-press hint.
        ui.update(Hardware::InputEvent::Press, 2100); ui.update(Hardware::InputEvent::Right, 2110);
        assert(ui.screen() == Ui::ScreenId::WebPlay && ui.takeAction() == Ui::UiAction::None);
        ui.update(Hardware::InputEvent::None, 4000); ui.render();
        snprintf(file, sizeof(file), "web-play-%s-hint", keys[role]); capture(ui, file);
        assert(textRowLit() && pixels != playing);
        // Long press asks the application to end the session, with the cancel sound.
        const unsigned cancels = cancelSounds;
        ui.update(Hardware::InputEvent::LongPress, 4100);
        assert(ui.takeAction() == Ui::UiAction::EndWebGame && cancelSounds == cancels + 1);
        ui.endWebPlay(pet.level(), 4100); ui.render();
        assert(ui.screen() == Ui::ScreenId::Home && ui.webGameStartAllowed());
        ui.endWebPlay(pet.level(), 4200);  // ignored away from the waiting screen
        assert(ui.screen() == Ui::ScreenId::Home);
    }
    {
        // A level gained by the reward celebrates on Home after the session.
        Hardware::Sound sound; auto pet = living(Pet::SpeciesId::Bird, Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        ui.beginWebPlay(2000);
        const uint8_t before = pet.level(); pet.gainExp(pet.expToNextLevel());
        ui.endWebPlay(before, 3000);
        assert(ui.screen() == Ui::ScreenId::Home && !ui.webGameStartAllowed());
        ui.update(Hardware::InputEvent::None, 3500); ui.update(Hardware::InputEvent::None, 5600);
        assert(ui.webGameStartAllowed());
    }
    {
        // Menus, the game picker and illness/death are not idle Home.
        Hardware::Sound sound; auto pet = living(Pet::SpeciesId::Wyvern, Pet::LifeStage::Adult);
        Ui::UiController ui(display, sound, pet, records, randomMove); home(ui);
        ui.update(Hardware::InputEvent::Press, 2100);
        assert(ui.screen() == Ui::ScreenId::MainMenu && !ui.webGameStartAllowed());
        for (unsigned i = 0; i < 3; ++i) ui.update(Hardware::InputEvent::Down, 2130 + i * 30);
        ui.update(Hardware::InputEvent::Press, 2250);
        assert(ui.screen() == Ui::ScreenId::GameSelect && ui.deviceGameActive() && !ui.webGameStartAllowed());
        home(ui); ui.beginWebPlay(2000);
        pet.setSick(true); pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds);
        assert(pet.isDead()); ui.update(Hardware::InputEvent::None, 2400);
        assert(ui.screen() == Ui::ScreenId::DeathAnimation);
    }
    puts("PASS: web play waiting screen for all four roles, alternating text, ignored input, long-press end, level-up after reward, idle-Home start rule and death interruption.");
}
