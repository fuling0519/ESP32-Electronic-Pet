// Runs the real UI/render code against the same U8g2 fonts in a host framebuffer.
#include <Arduino.h>
#include <u8g2.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <array>
#include "hardware/Display.h"
#include "hardware/Sound.h"
#include "storage/Memorials.h"
#include "ui/UiController.h"
#include "ui/UiFont12.h"
#include "ui/StatusFont12.h"
#include "pet/PetData.h"

SerialStub Serial;
static u8g2_t gfx{};
static u8x8_display_info_t info{};
static std::array<uint8_t, 1024> pixels{};
static uint32_t opponent = 0;
static unsigned failureSounds = 0;
static uint32_t randomMove() { return opponent; }
static void point(int x, int y) { assert(x >= 0 && x < 128 && y >= 0 && y < 64); }
static unsigned decode(const char*& s) {
    const unsigned c = static_cast<unsigned char>(*s++);
    if (c < 128) return c;
    const unsigned b = static_cast<unsigned char>(*s++);
    const unsigned a = static_cast<unsigned char>(*s++);
    return ((c & 15) << 12) | ((b & 63) << 6) | (a & 63);
}
namespace Hardware {
bool Display::isInitialized() const { return true; }
void Display::clear() { pixels.fill(0); }
void Display::update() {}
void Display::drawLine(int16_t x, int16_t y, int16_t xx, int16_t yy) {
    point(x,y); point(xx,yy); u8g2_DrawLine(&gfx,x,y,xx,yy);
}
void Display::drawFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
    point(x,y); point(x+w-1,y+h-1); u8g2_DrawFrame(&gfx,x,y,w,h);
}
void Display::clearArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    u8g2_SetDrawColor(&gfx,0); u8g2_DrawBox(&gfx,x,y,w,h); u8g2_SetDrawColor(&gfx,1);
}
void Display::drawText(int16_t x, int16_t y, const char* s) {
    u8g2_SetFont(&gfx,u8g2_font_6x10_tf); u8g2_DrawStr(&gfx,x,y,s);
}
void Display::drawSmallText(int16_t x, int16_t y, const char* s) {
    u8g2_SetFont(&gfx,u8g2_font_5x7_tf); u8g2_DrawStr(&gfx,x,y,s);
    u8g2_SetFont(&gfx,u8g2_font_6x10_tf);
}
void Display::drawGlyph(int16_t x, int16_t y, const uint8_t* bits, uint8_t w, uint8_t h) {
    point(x,y); point(x+w-1,y+h-1);
    for (unsigned row=0;row<h;++row) for(unsigned col=0;col<w;++col)
        if (!(bits[row*((w+7)/8)+col/8] & (128>>(col%8)))) u8g2_DrawPixel(&gfx,x+col,y+row);
}
void Display::drawUiText(int16_t x, int16_t y, const char* s) {
    while (*s) {
        unsigned c=decode(s);
        if(c<128) { char ch[2]={static_cast<char>(c),0}; drawText(x,y,ch); x+=6; }
        else {
            const uint8_t* bits=nullptr;
            for(unsigned i=0;i<UiFont12::kGlyphCount;++i)
                if(UiFont12::kGlyphs[i].codepoint==c) bits=UiFont12::kGlyphs[i].bitmap;
            assert(bits); drawGlyph(x,y-11,bits,12,12); x+=12;
        }
    }
}
uint16_t Display::uiTextWidth(const char* s) const {
    uint16_t w=0; while(*s) w+=decode(s)<128?6:12; return w;
}
void Display::drawStatusText(int16_t x,int16_t y,const char* s) {
    u8g2_SetFont(&gfx,u8g2_font_pet_status_12); u8g2_DrawUTF8(&gfx,x,y,s);
    u8g2_SetFont(&gfx,u8g2_font_6x10_tf);
}
uint16_t Display::statusTextWidth(const char* s) const {
    u8g2_SetFont(&gfx,u8g2_font_pet_status_12); auto w=u8g2_GetUTF8Width(&gfx,s);
    u8g2_SetFont(&gfx,u8g2_font_6x10_tf); return w;
}
void Sound::playConfirm() {} void Sound::playCancel() {}
void Sound::playSuccess() {} void Sound::playFailure() { ++failureSounds; }
void Sound::playHatch() {} void Sound::playDeath() {}
void Sound::playLevelUp() {}
void Sound::playTone(uint16_t,uint32_t) {}
}
static void capture(Ui::UiController& ui, const char* name) {
    ui.render();
    char path[160]; snprintf(path,sizeof(path),".pio/rps-preview/%s.pbm",name);
    FILE* f=fopen(path,"wb"); assert(f); fprintf(f,"P1\n128 64\n");
    for(unsigned y=0;y<64;++y) {
        for(unsigned x=0;x<128;++x) fprintf(f,"%u ",(pixels[(y/8)*128+x]>>(y%8))&1);
        fputc('\n',f);
    }
    fclose(f);
}
static bool lit(int x, int y) {
    return (pixels[(y / 8) * 128 + x] >> (y % 8)) & 1;
}
static void checkStatusSpacing() {
    int previousBottom = 18;
    unsigned groups = 0;
    for (int y = 19; y < 63;) {
        const auto rowLit = [](int row) {
            for (int x = 3; x <= 124; ++x) if (lit(x,row)) return true;
            return false;
        };
        if (!rowLit(y)) { ++y; continue; }
        assert(y - previousBottom - 1 == 5);
        while (y < 63 && rowLit(y)) ++y;
        previousBottom = y - 1;
        ++groups;
    }
    assert(groups == 3); // Level, EXP and progress frame.
}
int main() {
    info.tile_width=16; info.tile_height=8; info.pixel_width=128; info.pixel_height=64;
    gfx.u8x8.display_info=&info;
    u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);
    Hardware::Display display; Hardware::Sound sound;
    Pet::PetData pet; Storage::Memorials memorials;
    Ui::UiController ui(display,sound,pet,memorials,randomMove);
    using E=Hardware::InputEvent; using S=Ui::ScreenId; using A=Ui::UiAction;
    uint32_t now=1500;
    auto input=[&](E e, uint32_t delay=1) { now+=delay; ui.update(e,now); ui.render(); };
    ui.init(0); input(E::None); input(E::Press); input(E::Down); input(E::Down); input(E::Down);
    input(E::Press); assert(ui.screen()==S::PlayCare); capture(ui,"ready");
    input(E::Press); capture(ui,"select-rock");
    input(E::Left); capture(ui,"select-scissors");
    input(E::Right); input(E::Right); capture(ui,"select-paper");
    input(E::Right); input(E::Left); // Clamp at Paper, then select Rock.
    pet.setMood(96);
    pet.setExp(45);
    for(unsigned round=0;round<3;++round) {
        input(E::Press); capture(ui,"countdown-3");
        input(E::None,350); capture(ui,"countdown-2");
        input(E::None,350); capture(ui,"countdown-1");
        input(E::Press,350); capture(ui,"reveal-win");
        assert(ui.takeAction()==(round==2?A::FinishGame:A::None));
        assert(ui.takeAction()==A::None);
        if(round==2) {
            const unsigned reward=ui.takeGameReward(); assert(reward==15);
            const auto previousLevel = pet.level();
            pet.changeMood(reward);
            const auto gain = pet.gainExp(reward);
            ui.onGameRewardApplied(4, gain, previousLevel);
            assert(pet.level() == 2 && pet.exp() == 10);
            assert(pet.mood()==100 && ui.takeGameReward()==0);
        }
        input(E::Press,799); // Too early: must not skip reveal or deal a round.
        assert(ui.takeAction()==A::None);
        input(E::Press,1);
    }
    capture(ui,"summary-win-cap");
    input(E::Press); capture(ui,"level-up-0");
    for (unsigned frame=1;frame<16;++frame) {
        input(E::Press,125); // Animation consumes presses; replay remains untouched.
        char name[32]; snprintf(name,sizeof(name),"level-up-%u",frame); capture(ui,name);
        if (frame == 3) {
            assert(lit(18,23) && lit(109,37));
            assert(!lit(18,39) && !lit(109,25));
        }
        if (frame == 9) {
            assert(lit(18,39) && lit(109,25));
            assert(!lit(18,23) && !lit(109,37));
        }
        assert(ui.takeGameReward()==0 && pet.exp()==10);
    }
    input(E::Press,125); capture(ui,"replay-return");
    input(E::Up); capture(ui,"replay-again");
    input(E::Press); assert(ui.takeGameReward()==0);
    input(E::LongPress); assert(ui.screen()==S::MainMenu && ui.menuIndex()==3);
    // Full tie and loss sessions; completion at 100 mood displays +0.
    for(opponent=1;opponent<=2;++opponent) {
        input(E::Press); input(E::Press);
        for(unsigned round=0;round<3;++round) {
            input(E::Press); input(E::None,1050);
            capture(ui,opponent==1?"reveal-draw":"reveal-loss");
            if(round==2) {
                assert(ui.takeAction()==A::FinishGame);
                assert(ui.takeGameReward()==(opponent==1?10:5));
                ui.onGameRewardApplied(0);
            }
            input(E::Press,800);
        }
        capture(ui,opponent==1?"summary-draw":"summary-loss");
        input(E::Press); input(E::Press); assert(ui.screen()==S::MainMenu);
    }
    // Sickness interrupts a countdown and cannot settle it later.
    input(E::Press); input(E::Press); input(E::Press);
    pet.setSick(true); input(E::None,1050); capture(ui,"blocked-sick");
    assert(ui.takeAction()==A::None && ui.takeGameReward()==0);
    input(E::LongPress); pet.setSick(false);
    // Eggs cannot enter selection, even after pressing start.
    pet.startNewEgg(); input(E::None); input(E::Press); input(E::Press);
    capture(ui,"blocked-egg"); assert(ui.takeGameReward()==0);
    input(E::LongPress);
    auto snapshot=pet.snapshot(); snapshot.lifeStage=Pet::LifeStage::Baby;
    snapshot.ageSeconds=Pet::PetData::kAdultAgeSeconds-1; assert(pet.restore(snapshot));
    input(E::None); input(E::None,2400); // Existing hatch transition.
    input(E::Press); input(E::Press); input(E::Press); input(E::Press);
    pet.advanceSeconds(1); input(E::None,1050);
    assert(ui.screen()==S::GrowTransition && ui.takeGameReward()==0);
    input(E::None,2400); input(E::Press); input(E::Press); input(E::Press); input(E::Press);
    pet.setDead(true); input(E::None,1050);
    assert(ui.screen()==S::DeathAnimation && ui.takeGameReward()==0);
    // Real care actions award once; feeding completes before celebration.
    for (unsigned care = 0; care < 2; ++care) {
        Pet::PetData p;
        p.setExp(49); p.setSatiety(99); p.setCleanliness(99);
        if (care == 2) p.setSick(true);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500);
        view.update(E::Press,1501);
        for (unsigned i=0;i<care;++i) view.update(E::Down,1502+i);
        view.update(E::Press,1510); view.update(E::Press,1511);
        assert(view.takeAction() == (care==0 ? A::Feed : care==1 ? A::Clean : A::Treat));
        const auto old = p.level();
        assert(care==0 ? p.feed() : care==1 ? p.clean() : p.treat());
        if (care==0) view.onFeedSucceeded(1511);
        else if (care==1) view.onCleanSucceeded(1511);
        view.onCareRewardApplied(old,1511);
        assert(p.level()==2 && p.exp()==(care==2 ? 4 : 2));
        capture(view,care==0 ? "care-feed-before-up" : care==1 ? "care-clean-before-up" : "care-treat-before-up");
        const auto before = pixels;
        const uint32_t start = care==0 ? 4511 : 3912;
        view.update(E::Press,start-1); assert(view.takeAction()==A::None);
        view.update(E::Press,start); capture(view,"care-level-up");
        assert(pixels != before && view.takeAction()==A::None);
        const auto celebration = pixels;
        view.update(E::Press,start+1); view.render();
        assert(pixels==celebration && view.takeAction()==A::None);
        view.update(E::Press,start+2000);
        assert(view.takeAction()==A::None && p.exp()==(care==2 ? 4 : 2));
        view.update(E::None,start+2001); view.render();
        assert(pixels != celebration);
    }
    // Cleaning replaces only the center, preserves the live HUD/footer, locks
    // all input until 2400ms, and restores dirt from the current (not saved) value.
    for (unsigned baby=0; baby<2; ++baby) {
        Pet::PetData p;
        if (baby) { p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); }
        p.setCleanliness(40); p.setExp(25); p.setSick(true);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500);
        capture(view,baby?"clean-baby-before":"clean-adult-before");
        view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Press,1503);
        assert(view.screen()==S::CleanCare);
        view.update(E::Press,1504); assert(view.takeAction()==A::Clean);
        const auto old=p.level(); assert(p.clean());
        view.onCleanSucceeded(1504); view.onCareRewardApplied(old,1504);
        assert(view.screen()==S::Home && p.cleanliness()==70 && p.exp()==28);
        assert(view.takeAction()==A::None);
        capture(view,baby?"clean-baby-0":"clean-adult-0");
        const auto initial=pixels;
        for (unsigned frame=1; frame<4; ++frame) {
            view.update(E::Press,1504+frame*400);
            assert(view.screen()==S::Home && view.takeAction()==A::None);
            char name[32]; snprintf(name,sizeof(name),"clean-%s-%u",baby?"baby":"adult",frame);
            capture(view,name);
            assert(p.cleanliness()==70 && p.exp()==28);
            for(int y=0;y<64;++y) for(int x=0;x<128;++x) {
                if(x<20 || x>=111 || y>=55) {
                    const auto index=(y/8)*128+x;
                    assert((pixels[index] & (1<<(y%8))) == (initial[index] & (1<<(y%8))));
                }
            }
            if(frame==3) for(int y=3;y<=51;++y) for(int x=23;x<=106;++x) assert(lit(x,y));
        }
        const auto full=pixels;
        for (E event : {E::Up,E::Down,E::Left,E::Right,E::Press,E::LongPress}) {
            view.update(event,3903); view.render();
            assert(view.screen()==S::Home && view.takeAction()==A::None && pixels==full);
        }
        view.update(E::Press,3904); assert(view.screen()==S::Home && view.takeAction()==A::None);
        capture(view,baby?"clean-baby-restored":"clean-adult-restored");
        assert(lit(25,13) && p.cleanliness()==70);
        // Force a live change at the boundary: baby shows dirt, adult is clean.
        p.setCleanliness(baby?0:100);
        capture(view,baby?"clean-baby-live-dirt":"clean-adult-live-dirt");
        assert(lit(25,13)==bool(baby) && lit(117,8)); // Sickness is never cured by cleaning.
        view.update(E::Right,3905); view.update(E::Press,3906);
        assert(view.screen()==S::DetailedStatus);
    }
    // At 100%, animation still plays but grants no extra EXP.
    {
        Pet::PetData p; p.setCleanliness(100); p.setExp(25);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Press,1503); view.update(E::Press,1504);
        assert(view.takeAction()==A::Clean && p.clean()); view.onCleanSucceeded(1504);
        assert(p.cleanliness()==100 && p.exp()==25);
        view.update(E::Press,3904); view.update(E::None,3905);
        assert(view.screen()==S::Home && view.takeAction()==A::None && p.exp()==25);
        // Death has priority over the cleaning lock.
        view.onCleanSucceeded(4000); p.setDead(true); view.update(E::Press,4001);
        assert(view.screen()==S::DeathAnimation && view.takeAction()==A::None);
    }
    // Eggs and sleeping pets remain ineligible for the application action.
    {
        Pet::PetData p; p.startNewEgg();
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Press,1503); view.update(E::Press,1504);
        assert(view.screen()==S::CleanCare && view.takeAction()==A::None && !p.clean());
        p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); assert(p.beginNormalSleep());
        assert(!p.clean());
    }
    // Growth waits for cleaning to finish; unsigned clock wrap keeps the hold.
    {
        Pet::PetData p; p.startNewEgg(); p.advanceSeconds(Pet::PetData::kAdultAgeSeconds-1);
        assert(p.lifeStage()==Pet::LifeStage::Baby);
        p.setSick(false); p.setSatiety(100); p.setCleanliness(100);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500);
        view.onCleanSucceeded(UINT32_MAX-1199);
        p.advanceSeconds(1); assert(p.lifeStage()==Pet::LifeStage::Adult);
        view.update(E::Press,1199); assert(view.screen()==S::Home);
        view.update(E::Press,1200); assert(view.screen()==S::Home);
        view.update(E::None,1201); assert(view.screen()==S::GrowTransition);
    }
    // Real treatment timing, input lock, both sprites, live dirt restoration,
    // one-shot settlement and deferred upgrade/growth celebration.
    for (unsigned baby = 0; baby < 2; ++baby) {
        Pet::PetData p;
        if (baby) { p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); }
        p.setSick(true); p.setCleanliness(0); p.setExp(49);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.render();
        assert(lit(25,13)); // Existing upper-left dirt glyph.
        view.update(E::Press,1501); view.update(E::Down,1502); view.update(E::Down,1503);
        view.update(E::Press,1510); view.render();
        assert(view.screen()==S::Home && view.takeAction()==A::None && p.isSick());
        assert(!lit(25,13) && p.cleanliness()==0);
        const uint32_t offsets[]={0,250,500,850};
        for (unsigned f=0;f<4;++f) {
            view.update(E::Press,1510+offsets[f]);
            assert(view.takeAction()==A::None && p.isSick() && p.exp()==49);
            char name[64]; snprintf(name,sizeof(name),"treat-%s-pour-%u",baby?"baby":"adult",f);
            capture(view,name); assert(!lit(25,13));
        }
        view.update(E::LongPress,2709); assert(view.takeAction()==A::None && p.isSick());
        view.update(E::Press,2710); assert(view.takeAction()==A::Treat);
        assert(view.takeAction()==A::None); // Only one action at the boundary.
        const auto old=p.level(); assert(p.treat());
        view.onTreatResult(true,2710); view.onCareRewardApplied(old,2710);
        assert(!p.isSick() && p.level()==2 && p.exp()==4 && p.cleanliness()==0);
        capture(view,baby?"treat-baby-empty":"treat-adult-empty");
        assert(!lit(118,10)); // Sick cross disappears at settlement.
        p.setCleanliness(baby?0:100); // Read the live value when animation finishes.
        for (unsigned f=0;f<16;++f) {
            view.update(f%2?E::LongPress:E::Press,2910+f*125);
            assert(view.screen()==S::Home && view.takeAction()==A::None && p.exp()==4);
            char name[64]; snprintf(name,sizeof(name),"treat-%s-sparkle-%u",baby?"baby":"adult",f);
            capture(view,name); assert(!lit(25,13));
            if(f==3) { assert(lit(33,23) && lit(94,37)); }
        }
        view.update(E::Press,4910); capture(view,baby?"treat-baby-restored":"treat-adult-restored");
        assert(lit(25,13)==bool(baby) && p.cleanliness()==(baby?0:100));
        assert(view.takeAction()==A::None && p.exp()==4);
        view.update(E::None,4911); capture(view,"treat-following-level-up");
        view.update(E::LongPress,4912); assert(view.screen()==S::MainMenu);
    }
    // Death while pouring aborts treatment and emits no cure; healthy/egg blocked.
    {
        Pet::PetData p; p.setSick(true);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Down,1503); view.update(E::Press,1504);
        p.setDead(true); view.update(E::None,2704);
        assert(view.screen()==S::DeathAnimation && view.takeAction()==A::None);
        view.update(E::None,2705); assert(view.takeAction()==A::None);
    }
    for (unsigned egg=0;egg<2;++egg) {
        Pet::PetData p; if(egg) p.startNewEgg();
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Down,1503);
        view.render(); const auto menu = pixels;
        const unsigned failures = failureSounds;
        view.update(E::Press,1504);
        assert(view.screen()==S::MainMenu && view.menuIndex()==2);
        capture(view,egg?"treat-egg-not-needed":"treat-healthy-not-needed");
        assert(pixels != menu && failureSounds==failures);
        view.update(E::Press,1505); view.render(); // Repeat refreshes the notice.
        assert(pixels != menu && failureSounds==failures);
        view.update(E::None,2704); view.render(); assert(pixels != menu);
        view.update(E::None,2705); view.render(); assert(pixels == menu);
        view.update(E::Press,2706); view.update(E::Down,2707);
        assert(view.menuIndex()==3); // Navigation is never locked by the notice.
        view.update(E::Up,2708); view.render(); assert(pixels == menu);
        assert(view.takeAction()==A::None && p.exp()==0);
    }
    // Revalidate at settlement: a failed cure returns to the care screen.
    {
        Pet::PetData p; p.setSick(true);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Down,1503); view.update(E::Press,1504);
        p.setSick(false); view.update(E::None,2704); assert(view.takeAction()==A::Treat);
        const bool success=p.treat(); assert(!success); view.onTreatResult(success,2704);
        assert(view.screen()==S::MainMenu && view.menuIndex()==2 && p.exp()==0);
        capture(view,"treat-failed-not-needed");
    }
    // Curing a baby past its growth deadline finishes recovery before growth.
    {
        Pet::PetData p; p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
        p.setSick(true); p.advanceSeconds(Pet::PetData::kAdultAgeSeconds); p.setExp(49);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        view.update(E::Down,1502); view.update(E::Down,1503); view.update(E::Press,1504);
        view.update(E::None,2704); assert(view.takeAction()==A::Treat);
        const auto old=p.level(); assert(p.treat()); view.onTreatResult(true,2704);
        view.onCareRewardApplied(old,2704); assert(p.lifeStage()==Pet::LifeStage::Adult);
        view.update(E::None,2705); assert(view.screen()==S::Home);
        view.update(E::None,4904); assert(view.screen()==S::Home);
        view.update(E::None,4905); assert(view.screen()==S::GrowTransition);
        view.update(E::None,7305); assert(view.screen()==S::Home);
        view.update(E::None,7306); capture(view,"treat-growth-level-up");
        assert(view.takeAction()==A::None && p.exp()==4);
    }
    // Death cancels a queued care celebration; it must never cover the death flow.
    {
        Pet::PetData p; p.setExp(49); p.setSatiety(99);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500);
        const auto old=p.level(); assert(p.feed());
        view.onFeedSucceeded(1501); view.onCareRewardApplied(old,1501);
        p.setDead(true); view.update(E::None,1502);
        assert(view.screen()==S::DeathAnimation);
        capture(view,"care-death-priority"); const auto death=pixels;
        view.update(E::None,1503); view.render(); assert(pixels==death);
    }
    // A cure at the delayed growth boundary prioritizes growth, then celebrates.
    {
        Pet::PetData p; p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
        p.setSick(true); p.advanceSeconds(Pet::PetData::kAdultAgeSeconds);
        assert(p.lifeStage()==Pet::LifeStage::Baby); p.setExp(49);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500);
        const auto old=p.level(); assert(p.treat()); view.onCareRewardApplied(old,1501);
        view.update(E::None,1502); assert(view.screen()==S::GrowTransition);
        view.update(E::None,3902); assert(view.screen()==S::Home);
        view.update(E::None,3903); capture(view,"care-growth-level-up");
        view.update(E::LongPress,3904); assert(view.screen()==S::MainMenu && p.exp()==4);
        view.update(E::None,6000); assert(view.screen()==S::MainMenu);
    }
    // Footer grows with the current level's EXP, resets after level-up,
    // and remains full at MAX even though stored EXP is zero.
    Pet::PetData footerPet;
    Ui::UiController footer(display,sound,footerPet,memorials,randomMove);
    footer.init(0); footer.update(E::None,1500); capture(footer,"home-exp-0");
    assert(lit(3,59) && lit(3,60) && lit(3,61) && lit(101,60));
    assert(!lit(4,60) && !lit(100,60));
    footerPet.setExp(25); capture(footer,"home-exp-half");
    assert(lit(4,60) && lit(50,60) && !lit(60,60) && lit(101,60));
    footerPet.setExp(49); capture(footer,"home-exp-near");
    assert(lit(98,60) && !lit(100,60));
    assert(footerPet.gainExp(1)==1 && footerPet.level()==2 && footerPet.exp()==0);
    capture(footer,"home-exp-level-reset"); assert(!lit(4,60) && lit(101,60));
    footerPet.setLevel(Pet::PetData::kMaxLevel);
    capture(footer,"home-exp-max"); assert(lit(4,60) && lit(100,60) && lit(101,60));
    Pet::PetData eggPet; eggPet.startNewEgg();
    Ui::UiController eggFooter(display,sound,eggPet,memorials,randomMove);
    eggFooter.init(0); eggFooter.update(E::None,1500);
    capture(eggFooter,"home-exp-egg"); assert(!lit(4,60) && lit(101,60));
    // Exercise the same UI with baby sprites, a multi-level result and MAX.
    for (unsigned variant=0;variant<2;++variant) {
        Pet::PetData p;
        if (!variant) {
            p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds);
            p.setExp(45);
        } else p.setLevel(Pet::PetData::kMaxLevel);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        uint32_t t=1500;
        auto step=[&](E e, uint32_t delay=1) { t+=delay; view.update(e,t); view.render(); };
        view.init(0); step(E::None);
        step(E::Right); step(E::Press); step(E::Right);
        capture(view,variant?"status-health-stage-adult":"status-health-stage-baby");
        step(E::Right);
        capture(view,variant?"exp-max":"exp-progress");
        checkStatusSpacing();
        step(E::Right); capture(view,"status-age-fourth");
        step(E::LongPress); step(E::Press);
        step(E::Down); step(E::Down); step(E::Down); step(E::Press); step(E::Press);
        for(unsigned r=0;r<3;++r) {
            step(E::Press); step(E::None,1050);
            if (r==2) {
                assert(view.takeAction()==A::FinishGame);
                assert(view.takeGameReward()!=0);
                const auto old=p.level(); const auto added=p.gainExp(200);
                view.onGameRewardApplied(0,added,old);
            }
            step(E::Press,800);
        }
        capture(view,variant?"summary-max":"summary-multi-level"); step(E::Press);
        if (!variant) {
            assert(p.level()==4 && p.exp()==20);
            capture(view,"baby-level-up-0");
            for(unsigned f=1;f<16;++f) {
                step(E::None,125);
                char name[40]; snprintf(name,sizeof(name),"baby-level-up-%u",f); capture(view,name);
            }
            // Long press cancels just the display; awarded EXP remains.
            step(E::LongPress); assert(view.screen()==S::MainMenu && p.exp()==20);
        } else {
            capture(view,"max-replay"); step(E::Press);
            assert(view.screen()==S::MainMenu && p.exp()==0);
        }
    }
    puts("PASS: real UI flow, preview glyphs/bounds, single completion, cap, replay, egg/sick/growth/death interruptions.");
}
