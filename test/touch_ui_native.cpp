// Reuse the actual U8g2 Display adapter and existing UI sound instrumentation.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main
#include "HardwareConfig.h"
#include "pet/PetClock.h"
#include "ui/DirtySprite.h"

using Hardware::TouchEvent;
static Pet::PetData healthy(Pet::SpeciesId species, Pet::LifeStage stage) {
    Pet::PetData pet;
    assert(pet.startNewEgg(123, "Tamama", species));
    auto snapshot=pet.snapshot(); snapshot.lifeStage=stage;
    snapshot.ageSeconds=stage==Pet::LifeStage::Baby ? Pet::PetData::kEggHatchAgeSeconds : Pet::PetData::kAdultAgeSeconds;
    assert(pet.restore(snapshot)); return pet;
}
static void ready(Ui::UiController& ui) {
    ui.init(0); ui.update(Hardware::InputEvent::None,1500);
    ui.update(Hardware::InputEvent::None,2000); ui.render();
    assert(ui.touchContextAllowed());
}
int main() {
    info.tile_width=16; info.tile_height=8; info.pixel_width=128; info.pixel_height=64;
    gfx.u8x8.display_info=&info;
    u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);
    u8g2_SetDrawColor(&gfx,1);
    Hardware::Display display; Storage::Memorials records;
    const char* keys[] = {"bird-baby","bird-adult","wyvern-baby","wyvern-adult"};
    const unsigned counts[] = {14,13,8,8};
    for(unsigned role=0;role<4;++role) {
        Hardware::Sound sound;
        auto pet=healthy(role<2?Pet::SpeciesId::Bird:Pet::SpeciesId::Wyvern,
                         role%2?Pet::LifeStage::Adult:Pet::LifeStage::Baby);
        Ui::UiController ui(display,sound,pet,records,randomMove); ready(ui);
        auto before=pet.snapshot();
        char file[100]; snprintf(file,sizeof(file),"touch-%s-base",keys[role]); capture(ui,file);
        const auto base=pixels;
        const unsigned sounds=pettingSounds;
        assert(ui.onTouchEvent(TouchEvent::PettingStarted,2000));
        assert(!ui.onTouchEvent(TouchEvent::PettingStarted,2001));
        assert(!ui.onTouchEvent(TouchEvent::Poke,2001));
        assert(pettingSounds==sounds+1);
        const unsigned fps=role<2?5:7;
        ui.render(); const auto first=pixels;
        ui.update(Hardware::InputEvent::None,2000+(1000+fps-1)/fps-1); ui.render();
        assert(pixels==first);
        assert(Ui::PetIcons::touchHappyFrameCount(pet.lifeStage(),pet.speciesId())==counts[role]);
        for(unsigned frame=0;frame<counts[role];++frame) {
            ui.update(Hardware::InputEvent::None,2000+(frame*1000+fps-1)/fps);
            snprintf(file,sizeof(file),"touch-%s-happy-%u",keys[role],frame); capture(ui,file);
            for(unsigned y=0;y<64;++y) for(unsigned x=0;x<128;++x)
                if(x<32 || x>=96 || y<6 || y>=50)
                    assert(lit(x,y)==bool((base[(y/8)*128+x]>>(y%8))&1));
        }
        ui.update(Hardware::InputEvent::None,7000);
        assert(!ui.onTouchEvent(TouchEvent::PettingStarted,7001)); // five-second cap tail
        ui.update(Hardware::InputEvent::None,7249);
        assert(!ui.onTouchEvent(TouchEvent::Poke,7249));
        ui.update(Hardware::InputEvent::None,7250);
        assert(!ui.onTouchEvent(TouchEvent::PettingReleased,7250));
        assert(ui.onTouchEvent(TouchEvent::Poke,8000));
        ui.update(Hardware::InputEvent::None,8000);
        snprintf(file,sizeof(file),"touch-%s-shock-0",keys[role]); capture(ui,file);
        assert(!ui.onTouchEvent(TouchEvent::Poke,8100));
        ui.update(Hardware::InputEvent::None,8160);
        snprintf(file,sizeof(file),"touch-%s-shock-1",keys[role]); capture(ui,file);
        ui.update(Hardware::InputEvent::None,8500);
        if(role%2==0) {
            snprintf(file,sizeof(file),"touch-%s-recovered",keys[role]); capture(ui,file);
        }
        assert(pet.snapshot().mood==before.mood && pet.exp()==before.exp &&
               pet.satiety()==before.satiety && pet.cleanliness()==before.cleanliness &&
               pet.ageSeconds()==before.ageSeconds);
        assert(ui.takeAction()==Ui::UiAction::None);
        // An early release uses a continuous 250ms tail, not another event.
        assert(ui.onTouchEvent(TouchEvent::PettingStarted,9000));
        assert(ui.onTouchEvent(TouchEvent::PettingReleased,9400));
        assert(!ui.onTouchEvent(TouchEvent::PettingReleased,9401));
        ui.update(Hardware::InputEvent::None,9649);
        assert(!ui.onTouchEvent(TouchEvent::Poke,9649));
        ui.update(Hardware::InputEvent::None,9650);
        assert(ui.onTouchEvent(TouchEvent::Poke,9650));
        // Joystick input cancels immediately even when focus stays on Home.
        const auto context=ui.touchContextRevision();
        ui.update(Hardware::InputEvent::Right,9651);
        assert(ui.touchContextRevision()!=context && ui.touchContextAllowed());
        assert(ui.onTouchEvent(TouchEvent::PettingStarted,9652));
        ui.update(Hardware::InputEvent::Press,9653);
        assert(ui.screen()==Ui::ScreenId::DetailedStatus && !ui.touchContextAllowed());
        assert(!ui.onTouchEvent(TouchEvent::Poke,9654));
        ready(ui);
        // Slightly dirty pets retain the existing dirt; touch never cleans.
        pet.setCleanliness(60); ui.update(Hardware::InputEvent::None,2010); ui.render();
        assert(ui.onTouchEvent(TouchEvent::PettingStarted,2010)); ui.render();
        for(unsigned y=0;y<Ui::PetIcons::kDirtHeight;++y)
            for(unsigned x=0;x<Ui::PetIcons::kDirtWidth;++x)
                if(!(Ui::PetIcons::kDirtFrames[1][y*3+x/8] & (128>>(x%8)))) assert(lit(22+x,6+y));
        const auto beforeNeed=pet.snapshot();
        Pet::PetClock clock; clock.reset(2010);
        pet.advanceSeconds(clock.consumeElapsedSeconds(4010));
        ui.update(Hardware::InputEvent::None,4010);
        assert(pet.ageSeconds()==beforeNeed.ageSeconds+2 && pet.cleanliness()==60);
        pet.setMood(40); ui.update(Hardware::InputEvent::None,4020);
        assert(!ui.touchContextAllowed() && !ui.onTouchEvent(TouchEvent::Poke,4020));
        pet.setMood(44); ui.update(Hardware::InputEvent::None,4030); assert(!ui.touchContextAllowed());
        pet.setMood(45); ui.update(Hardware::InputEvent::None,4040); assert(ui.touchContextAllowed());
        pet.setSick(true); ui.update(Hardware::InputEvent::None,4050);
        assert(!ui.touchContextAllowed() && !ui.onTouchEvent(TouchEvent::PettingStarted,4050));
    }
    {
        Hardware::Sound sound; auto pet=healthy(Pet::SpeciesId::Bird,Pet::LifeStage::Baby);
        Ui::UiController ui(display,sound,pet,records,randomMove); ready(ui);
        const unsigned prior=displayUpdates;
        assert(ui.onTouchEvent(TouchEvent::PettingStarted,2000)); ui.render();
        assert(displayUpdates==prior+1);
        ui.update(Hardware::InputEvent::None,2100); ui.render(); assert(displayUpdates==prior+1);
        ui.update(Hardware::InputEvent::None,2200); ui.render(); assert(displayUpdates==prior+2);
        ui.onTouchEvent(TouchEvent::Cancelled,2201);
        assert(ui.onTouchEvent(TouchEvent::PettingStarted,2202)); // fault immediately ended previous visual
        const auto context=ui.touchContextRevision(); ui.onFeedSucceeded(2300);
        assert(ui.touchContextRevision()!=context && !ui.touchContextAllowed());
        assert(!ui.onTouchEvent(TouchEvent::Poke,2300));
        ready(ui); ui.onCleanSucceeded(2000); assert(!ui.touchContextAllowed());
        ui.update(Hardware::InputEvent::None,4400); assert(ui.touchContextAllowed());
        const auto level=pet.level(); pet.gainExp(pet.expToNextLevel());
        ui.onCareRewardApplied(level,4400); assert(!ui.touchContextAllowed());
        ui.update(Hardware::InputEvent::None,4800); assert(!ui.touchContextAllowed());
        ui.update(Hardware::InputEvent::None,6800); assert(ui.touchContextAllowed());
        ui.onDeepSleepPending(true); assert(!ui.touchContextAllowed());
        ui.onDeepSleepPending(false); assert(ui.touchContextAllowed());
        assert(pet.beginNormalSleep()); ui.onSleepStarted(7000);
        assert(!ui.touchContextAllowed() && !ui.onTouchEvent(TouchEvent::Poke,7000));
        assert(pet.wake()); ui.onWakeSucceeded();
        assert(ui.touchContextAllowed());
        pet.setSick(true); ui.update(Hardware::InputEvent::None,7200);
        ui.update(Hardware::InputEvent::Press,7210); // menu
        ui.update(Hardware::InputEvent::Down,7220); ui.update(Hardware::InputEvent::Down,7230);
        ui.update(Hardware::InputEvent::Press,7240); // treatment on Home
        assert(ui.screen()==Ui::ScreenId::Home && !ui.touchContextAllowed());
        assert(!ui.onTouchEvent(TouchEvent::PettingStarted,7240));
    }
    {
        Hardware::Sound sound; Pet::PetData egg;
        assert(egg.startNewEgg(124,"Tamama",Pet::SpeciesId::Bird));
        Ui::UiController ui(display,sound,egg,records,randomMove);
        ui.init(0); assert(!ui.touchContextAllowed());
        ui.update(Hardware::InputEvent::None,1500); assert(!ui.touchContextAllowed());
        auto pet=healthy(Pet::SpeciesId::Bird,Pet::LifeStage::Baby);
        Ui::UiController grown(display,sound,pet,records,randomMove); ready(grown);
        assert(grown.onTouchEvent(TouchEvent::PettingStarted,2000));
        auto s=pet.snapshot(); s.lifeStage=Pet::LifeStage::Adult; s.ageSeconds=Pet::PetData::kAdultAgeSeconds;
        assert(pet.restore(s)); grown.update(Hardware::InputEvent::None,2100);
        assert(grown.screen()==Ui::ScreenId::GrowTransition && !grown.touchContextAllowed());
        grown.update(Hardware::InputEvent::None,4500); assert(grown.touchContextAllowed());
        assert(grown.onTouchEvent(TouchEvent::PettingStarted,4500));
        assert(pet.depart()); grown.onFarewellResult(true,4600); assert(!grown.touchContextAllowed());
    }
    {
        Hardware::Sound sound; auto pet=healthy(Pet::SpeciesId::Wyvern,Pet::LifeStage::Adult);
        Ui::UiController ui(display,sound,pet,records,randomMove); ready(ui);
        ui.update(Hardware::InputEvent::Press,2100);
        for(unsigned i=0;i<3;++i) ui.update(Hardware::InputEvent::Down,2130+i*30);
        ui.update(Hardware::InputEvent::Press,2250);
        assert(ui.screen()==Ui::ScreenId::GameSelect && !ui.touchContextAllowed());
        ui.update(Hardware::InputEvent::Press,2300);
        assert(ui.screen()==Ui::ScreenId::PlayCare && !ui.touchContextAllowed());
        assert(!ui.onTouchEvent(TouchEvent::Poke,2300));
        ready(ui); assert(ui.onTouchEvent(TouchEvent::PettingStarted,2000));
        pet.setSick(true); pet.advanceSeconds(Pet::PetData::kDeathAfterSickAwakeSeconds);
        assert(pet.isDead()); ui.update(Hardware::InputEvent::None,2400);
        assert(ui.screen()==Ui::ScreenId::DeathAnimation && !ui.touchContextAllowed());
    }
    puts("PASS: real UI touch frames/HUD, one-shot reactions, dirty flag, cap/tail, navigation, care, sleep, sad/sick, growth/farewell and unchanged pet values.");
}
