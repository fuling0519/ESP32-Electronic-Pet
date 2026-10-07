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
static unsigned noteTones = 0;
static uint16_t lastNoteTone = 0;
static bool holdNotePlaying = false;
static unsigned successSounds = 0;
static unsigned careReminders = 0;
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
            if (!bits) fprintf(stderr, "Missing UI glyph U+%04X\n", c);
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
void Sound::playSuccess() { ++successSounds; } void Sound::playFailure() { ++failureSounds; }
void Sound::playHatch() {} void Sound::playDeath() {}
void Sound::playLevelUp() {}
void Sound::playCareReminder() { ++careReminders; }
void Sound::stopTone() { playing_=false; }
void Sound::playTone(uint16_t frequency,uint32_t) {
    ++noteTones; lastNoteTone=frequency; playing_=holdNotePlaying;
}
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
    input(E::Press); assert(ui.screen()==S::GameSelect); capture(ui,"game-select");
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
            const unsigned reward=ui.takeGameReward(); assert(reward==10);
            assert(ui.gameExpReward()==15);
            const auto previousLevel = pet.level();
            pet.changeMood(reward);
            const auto gain = pet.gainExp(ui.gameExpReward());
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
        input(E::Press); input(E::Press); input(E::Press);
        for(unsigned round=0;round<3;++round) {
            input(E::Press); input(E::None,1050);
            capture(ui,opponent==1?"reveal-draw":"reveal-loss");
            if(round==2) {
                assert(ui.takeAction()==A::FinishGame);
                assert(ui.takeGameReward()==(opponent==1?6:3));
                assert(ui.gameExpReward()==(opponent==1?10:5));
                const auto old=pet.level();
                const auto gain=pet.gainExp(ui.gameExpReward());
                assert(gain==(opponent==1?10:5));
                ui.onGameRewardApplied(0,gain,old);
                assert(ui.takeGameReward()==0);
            }
            input(E::Press,800);
        }
        capture(ui,opponent==1?"summary-draw":"summary-loss");
        input(E::Press); input(E::Press); assert(ui.screen()==S::MainMenu);
    }
    // Sickness interrupts a countdown and cannot settle it later.
    input(E::Press); input(E::Press); input(E::Press); input(E::Press);
    pet.setSick(true); input(E::None,1050); capture(ui,"blocked-sick");
    assert(ui.takeAction()==A::None && ui.takeGameReward()==0);
    input(E::LongPress); pet.setSick(false);
    // Eggs cannot enter selection, even after pressing start.
    pet.startNewEgg(); input(E::None); input(E::Press); input(E::Press); input(E::Press);
    capture(ui,"blocked-egg"); assert(ui.takeGameReward()==0);
    input(E::LongPress);
    auto snapshot=pet.snapshot(); snapshot.lifeStage=Pet::LifeStage::Baby;
    snapshot.ageSeconds=Pet::PetData::kAdultAgeSeconds-1; assert(pet.restore(snapshot));
    input(E::None); input(E::None,2400); // Existing hatch transition.
    input(E::Press); input(E::Press); input(E::Press); input(E::Press); input(E::Press);
    pet.advanceSeconds(1); input(E::None,1050);
    assert(ui.screen()==S::GrowTransition && ui.takeGameReward()==0);
    input(E::None,2400); input(E::Press); input(E::Press); input(E::Press); input(E::Press); input(E::Press);
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
        assert(view.screen()==S::MainMenu && view.takeAction()==A::None && !p.clean());
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
        if (egg) {
            assert(pixels == menu && failureSounds == failures + 1);
            assert(view.takeAction()==A::None && p.exp()==0);
            continue;
        }
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
        step(E::Down); step(E::Down); step(E::Down); step(E::Press); step(E::Press); step(E::Press);
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
    // Play the real memory-note UI at every score tier. Same application reward
    // gate as main.cpp, with actual fonts and framebuffer bounds checks above.
    for(unsigned score=0;score<=5;++score) {
        Pet::PetData p;
        p.setMood(score==5?96:80); p.setExp(45);
        Ui::UiController view(display,sound,p,memorials,randomMove);
        uint32_t t=1500;
        auto step=[&](E e,uint32_t delay=1,bool neutral=true) {
            t+=delay; view.update(e,t,neutral); view.render();
        };
        opponent=0;
        view.init(0); step(E::None); step(E::Press);
        step(E::Down); step(E::Down); step(E::Down); step(E::Press);
        assert(view.screen()==S::GameSelect);
        step(E::LongPress); assert(view.screen()==S::MainMenu && view.menuIndex()==3);
        step(E::Press); step(E::Down); capture(view,"notes-select");
        step(E::Press); capture(view,"notes-ready");
        assert(view.screen()==S::PlayCare);
        step(E::Press);
        for(unsigned round=1;round<=5;++round) {
            const E dirs[]={E::Left,E::Up,E::Right,E::Down};
            const uint16_t frequencies[]={523,587,659,784};
            const E correct=dirs[(round-1)%4];
            assert(lastNoteTone==frequencies[(round-1)%4]);
            char name[40];
            if(round==5) snprintf(name,sizeof(name),"notes-play-six");
            else snprintf(name,sizeof(name),"notes-play-%u",round-1);
            capture(view,name);
            for(unsigned i=0;i<round+1;++i) {
                const auto tones=noteTones;
                step(E::Right,250,false); // Playback ignores answers.
                if(round==1 && i==0) capture(view,"notes-gap");
                if(round==1 && i==1) capture(view,"notes-gap-last");
                step(E::Press,450,false);
                if(round==1 && i==0) capture(view,"notes-play-last");
                assert(view.takeAction()==A::None);
                assert(noteTones==tones+(i+1<round+1?1:0));
            }
            if(round==1) capture(view,"notes-answer");
            step(correct,400,false); // Held from playback: not an answer.
            assert(view.takeAction()==A::None);
            step(E::None,1,true);
            if(round<=score) {
                for(unsigned i=0;i<round+1;++i) {
                    const auto tones=noteTones, successes=successSounds;
                    holdNotePlaying=i==round;
                    step(correct,1,false);
                    if(i==round) {
                        assert(noteTones==tones+1 && lastNoteTone==frequencies[(round-1)%4]);
                        assert(successSounds==successes && view.takeAction()==A::None);
                        capture(view,"notes-final-answer");
                        if(round==1) capture(view,"notes-final-answer-first");
                        const auto finalPixels=pixels;
                        step(E::Press,179);
                        assert(pixels==finalPixels && successSounds==successes);
                        step(E::Press,1); // Hardware tone still playing: keep waiting.
                        assert(pixels==finalPixels && view.takeAction()==A::None);
                        sound.stopTone(); holdNotePlaying=false;
                        step(E::Press,1);
                        assert(successSounds==successes+1 && pixels!=finalPixels);
                    }
                    if(i+1<round+1) {
                        if(round==1 && i==0) {
                            capture(view,"notes-answer-flash");
                            assert(lit(36,33) && !lit(44,39));
                        }
                        step(correct,200,false); // Repeat is ignored.
                        assert(view.takeAction()==A::None);
                        step(E::None,1,true);
                    }
                }
                capture(view,"notes-correct");
                if(round==1) capture(view,"notes-correct-first");
            } else if(score==0) {
                step(E::None,5000,true); capture(view,"notes-timeout");
            } else {
                step(dirs[round%4],1,false); capture(view,"notes-wrong");
            }
            assert(view.takeAction()==(round==5?A::FinishGame:A::None));
            assert(view.takeAction()==A::None);
            if(round==5) {
                const uint8_t reward=view.takeGameReward();
                assert(reward==(score==5?10:score==4?8:score>=2?6:3));
                const auto old=p.level(), before=p.mood();
                p.changeMood(reward);
                const auto gain=p.gainExp(view.gameExpReward());
                assert(gain==(score>=4?15:score>=2?10:5));
                view.onGameRewardApplied(p.mood()-before,gain,old);
                assert(p.mood()==(score==5?100:80+reward));
                assert(view.takeGameReward()==0);
            }
            opponent=round%4;
            step(E::Press,799); assert(view.takeAction()==A::None);
            step(E::Press,1);
        }
        char summary[40]; snprintf(summary,sizeof(summary),"notes-summary-%u",score);
        capture(view,summary);
        step(E::Press); capture(view,"notes-level-up");
        step(E::Press,2000); assert(view.takeGameReward()==0);
        capture(view,"notes-replay");
        step(E::Up); step(E::Press); // Fresh game, no residual score/reward.
        assert(view.takeGameReward()==0);
        step(E::LongPress); assert(view.screen()==S::MainMenu && view.menuIndex()==3);
        assert(p.exp()==(score>=4?10:score>=2?5:0));
    }
    // Health, growth and death interrupt memory playback without a reward.
    for(unsigned interrupt=0;interrupt<4;++interrupt) {
        Pet::PetData p;
        if(interrupt==2) {
            p.startNewEgg(); p.advanceSeconds(Pet::PetData::kAdultAgeSeconds-1);
        }
        Ui::UiController view(display,sound,p,memorials,randomMove);
        uint32_t t=1500;
        auto step=[&](E e,uint32_t delay=1) { t+=delay; view.update(e,t); view.render(); };
        view.init(0); step(E::None); step(E::Press);
        step(E::Down); step(E::Down); step(E::Down); step(E::Press); step(E::Down); step(E::Press);
        if(interrupt==0) {
            p.startNewEgg(); step(E::None); capture(view,"notes-blocked-egg");
            step(E::Press); assert(view.takeGameReward()==0);
        } else {
            step(E::Press);
            if(interrupt==1) p.setSick(true);
            else if(interrupt==2) p.advanceSeconds(1);
            else p.setDead(true);
            step(E::None);
            assert(view.takeAction()==A::None && view.takeGameReward()==0);
            if(interrupt==1) capture(view,"notes-blocked-sick");
            if(interrupt==2) assert(view.screen()==S::GrowTransition);
            if(interrupt==3) assert(view.screen()==S::DeathAnimation);
        }
    }
    puts("PASS: memory-note UI score tiers, notes/blank timing, neutral gate/repeats, timeout, reward/EXP/cap/level-up, replay and interruptions.");
    // Farewell uses the real UI and framebuffer: viewfinder, flash and postcard.
    for (bool baby : {false, true}) for (bool sick : {false, true}) {
        Pet::PetData p;
        if (baby) { p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); }
        p.setSick(sick);
        Storage::Memorials album;
        Ui::UiController view(display,sound,p,album,randomMove);
        uint32_t t=1500;
        auto step=[&](E e,uint32_t delay=1) { t+=delay; view.update(e,t); view.render(); };
        view.init(0); step(E::None); step(E::Press); step(E::Up); step(E::Press);
        assert(view.screen()==S::FarewellInfo); capture(view,"farewell-info");
        step(E::Press); assert(view.screen()==S::FarewellConfirm);
        capture(view,"farewell-confirm-keep");
        step(E::Press); assert(view.screen()==S::MainMenu && view.takeAction()==A::None);
        step(E::Press); step(E::Press); step(E::Right);
        capture(view,"farewell-confirm-send"); step(E::Press);
        assert(view.takeAction()==A::SendOff);
        assert(p.depart()); assert(album.append(p));
        view.setMemorialReady(true); view.onFarewellResult(true,t);
        view.render(); capture(view,baby?"farewell-baby-start":"farewell-adult-start");
        for(unsigned elapsed=100;elapsed<=2700;elapsed+=100) {
            step(E::Press,100);
            assert(view.takeAction()==A::None);
            char frameName[64];
            snprintf(frameName,sizeof(frameName),"farewell-%s-%02u",baby?"baby":"adult",elapsed/100);
            capture(view,frameName);
            if(elapsed==2500) {
                view.update(E::Press,t+99); view.render();
                assert(view.screen()==S::FarewellAnimation && !lit(0,0));
                assert(view.takeAction()==A::None); // 2599ms still poses.
            }
            if(elapsed==1600) capture(view,baby?"farewell-baby-blink":"farewell-adult-blink");
            if(elapsed==2600) {
                for(int y=0;y<64;++y) for(int x=0;x<128;++x) assert(lit(x,y));
                capture(view,"farewell-flash");
                view.update(E::Press,t+99); view.render();
                assert(view.screen()==S::FarewellAnimation && view.takeAction()==A::None);
                for(int y=0;y<64;++y) for(int x=0;x<128;++x) assert(lit(x,y));
            }

        }
        assert(view.screen()==S::FarewellDone); capture(view,"farewell-done");
        capture(view,baby?"farewell-baby-card":"farewell-adult-card");
        for(int x=2;x<=125;++x) { assert(lit(x,2)); assert(lit(x,61)); }
        assert(lit(64,8) && lit(64,55) && !lit(64,7) && !lit(64,56));
        assert(lit(75,38) && lit(120,38) && lit(75,55));
        assert(lit(120,7) && !lit(121,7)); // Narrowed stamp right edge.
        const auto cardPixels = pixels;
        step(E::None,5000); assert(view.screen()==S::FarewellDone && pixels==cardPixels);
        step(E::LongPress); assert(view.screen()==S::FarewellDone && pixels==cardPixels);
        step(E::Press); assert(view.screen()==S::DeathOptions);
        step(E::Press); assert(view.screen()==S::MemorialCategories);
        capture(view,"album-categories");
        step(E::Press); assert(view.screen()==S::Graveyard);
        capture(view,baby?"album-baby":"album-adult");
        // Portrait feet must be clear of the bottom border, for both stages.
        for(int x=3;x<67;++x) { assert(!lit(x,61)); assert(!lit(x,62)); }
        int portraitTop=64, portraitBottom=-1;
        for(int y=19;y<=60;++y) for(int x=3;x<67;++x) if(lit(x,y)) {
            if(y<portraitTop) portraitTop=y;
            if(y>portraitBottom) portraitBottom=y;
        }
        assert(portraitBottom>=portraitTop && portraitTop+portraitBottom>=78 && portraitTop+portraitBottom<=80);
        step(E::Press); assert(view.screen()==S::Graveyard); // Current record protected.
        step(E::LongPress); step(E::Press);
        assert(view.screen()==S::Graveyard); // Departed filter.
        capture(view,baby?"album-baby-filter":"album-adult-filter");
        step(E::LongPress); step(E::Down); step(E::Press); capture(view,"album-resting-empty");
        step(E::Press); assert(view.screen()==S::Graveyard && view.takeAction()==A::None);
        step(E::LongPress); step(E::Up); step(E::Press); assert(view.selectedMemorialIndex()==0);
        step(E::LongPress); step(E::LongPress); step(E::Down); step(E::Press);
        assert(view.takeAction()==A::AdoptNewEgg);
        assert(p.startNewEgg(2,"New")); view.onAdoptionSucceeded(t);
        assert(view.screen()==S::Home);
        // Reboot a departed pet enters the farewell result, never Home/death.
        Pet::PetData away; assert(away.depart());
        Ui::UiController reboot(display,sound,away,album,randomMove);
        reboot.setMemorialReady(true); reboot.init(0); reboot.update(E::None,1500);
        assert(reboot.screen()==S::FarewellDone);
        reboot.setMemorialReady(false); reboot.init(0); reboot.update(E::None,1500);
        assert(reboot.screen()==S::FarewellBlocked);
        reboot.update(E::LongPress,1501); assert(reboot.screen()==S::FarewellBlocked);
        reboot.update(E::Press,1502); assert(reboot.takeAction()==A::RetryFarewell);
        reboot.setMemorialReady(true); reboot.update(E::None,1503);
        assert(reboot.screen()==S::FarewellDone);
    }
    for (unsigned scenario=0;scenario<3;++scenario) {
        Pet::PetData p; Storage::Memorials album;
        if(scenario==0) p.startNewEgg();
        if(scenario==1) for(unsigned i=0;i<Storage::kMemorialLimit;++i) {
            Storage::MemorialRecord r{}; r.petId=i+10; strcpy(r.name,"Old");
            r.speciesId=Pet::SpeciesId::Bird; assert(album.append(r));
        }
        Ui::UiController view(display,sound,p,album,randomMove);
        uint32_t t=1500;
        auto step=[&](E e) { view.update(e,++t); view.render(); };
        view.init(0); step(E::None); step(E::Press); step(E::Up); step(E::Press);
        if(scenario==0) assert(view.screen()==S::MainMenu);
        if(scenario==1) {
            assert(view.screen()==S::FarewellBlocked); capture(view,"farewell-full");
            step(E::Press); assert(view.screen()==S::MemorialCategories);
            step(E::Down); step(E::Press); assert(view.screen()==S::Graveyard); capture(view,"album-resting");
            step(E::Press); assert(view.screen()==S::DeleteMemorialConfirm);
            step(E::Press); assert(view.screen()==S::Graveyard && view.takeAction()==A::None);
            step(E::Press); step(E::Left); step(E::Press);
            assert(view.takeAction()==A::DeleteMemorial);
            assert(album.remove(view.selectedMemorialIndex())); view.onMemorialDeleteResult(true);
        }
        if(scenario==2) {
            step(E::Press); p.setDead(true); step(E::Right);
            assert(view.screen()==S::DeathAnimation && view.takeAction()==A::None);
        }
    }
    // Egg menu rejects every unusable action without a page or notice.
    for (unsigned item=0; item<8; ++item) {
        Pet::PetData p; p.startNewEgg();
        Ui::UiController view(display,sound,p,memorials,randomMove);
        view.init(0); view.update(E::None,1500); view.update(E::Press,1501);
        for(unsigned n=0;n<item;++n) view.update(E::Down,1502+n);
        view.render(); const auto menu=pixels; const unsigned failures=failureSounds;
        view.update(E::Press,1520); view.render();
        if(item<=4 || item==7) {
            assert(view.screen()==S::MainMenu && view.menuIndex()==item && pixels==menu);
            assert(failureSounds==failures+1 && view.takeAction()==A::None);
        } else assert(view.screen()==(item==5?S::DetailedStatus:S::MemorialCategories));
    }
    // Mixed records: categories, help, horizontal paging, empty filter and deletion.
    {
        Pet::PetData p; Storage::Memorials album;
        for(unsigned i=0;i<3;++i) {
            Storage::MemorialRecord r{}; r.petId=100+i; strcpy(r.name,"Old");
            r.speciesId=Pet::SpeciesId::Bird; r.stage=static_cast<uint8_t>(Pet::LifeStage::Adult);
            r.kind=i==1?Storage::FarewellKind::Resting:Storage::FarewellKind::Departed;
            if(i==1) { strcpy(r.name,"TestRIP"); r.ageSeconds=266400; }
            assert(album.append(r));
        }
        Ui::UiController view(display,sound,p,album,randomMove);
        uint32_t t=1500;
        auto step=[&](E e) { view.update(e,++t); view.render(); };
        view.init(0); step(E::None); step(E::Press); step(E::Up); step(E::Up); step(E::Press);
        assert(view.screen()==S::MemorialCategories); capture(view,"album-categories-mixed");
        step(E::Up); step(E::Press); assert(view.screen()==S::MemorialHelp);
        capture(view,"album-help"); step(E::LongPress); assert(view.screen()==S::MemorialCategories);
        step(E::Down); step(E::Press); assert(view.selectedMemorialIndex()==0);
        step(E::Right); assert(view.selectedMemorialIndex()==2);
        step(E::Right); assert(view.selectedMemorialIndex()==0);
        step(E::Left); assert(view.selectedMemorialIndex()==2);
        step(E::Down); assert(view.selectedMemorialIndex()==2);
        step(E::Press); assert(view.screen()==S::DeleteMemorialConfirm);
        capture(view,"album-delete-confirm"); step(E::Press);
        assert(view.screen()==S::Graveyard && view.takeAction()==A::None);
        step(E::LongPress); step(E::Press); // Departed only.
        assert(view.selectedMemorialIndex()==0); step(E::Right); assert(view.selectedMemorialIndex()==2);
        capture(view,"album-horizontal-record");
        step(E::Press); step(E::Left); step(E::Press); assert(view.takeAction()==A::DeleteMemorial);
        assert(album.remove(view.selectedMemorialIndex())); view.onMemorialDeleteResult(true);
        assert(view.screen()==S::Graveyard && view.selectedMemorialIndex()==0);
        step(E::LongPress); step(E::Down); step(E::Press); assert(view.selectedMemorialIndex()==1);
        capture(view,"album-resting-sample");
        step(E::Press); step(E::Left); step(E::Press); assert(view.takeAction()==A::DeleteMemorial);
        assert(album.remove(view.selectedMemorialIndex())); view.onMemorialDeleteResult(true);
        capture(view,"album-empty-after-delete"); assert(view.screen()==S::Graveyard);
        step(E::Press); assert(view.takeAction()==A::None);
        step(E::LongPress); step(E::LongPress); assert(view.screen()==S::MainMenu);
    }
    puts("PASS: egg menu restrictions, memorial categories/help/horizontal paging and delete confirmation.");
    // Full UI renders for pixel comparison with all four approved compositions.
    for(bool baby : {false,true}) for(bool resting : {false,true}) {
        Pet::PetData p; Storage::Memorials album;
        if(baby) { p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); }
        if(resting) {
            Storage::MemorialRecord r{}; r.petId=100; strcpy(r.name,"TestRIP");
            r.speciesId=Pet::SpeciesId::Bird; r.kind=Storage::FarewellKind::Resting;
            r.stage=static_cast<uint8_t>(baby?Pet::LifeStage::Baby:Pet::LifeStage::Adult);
            r.ageSeconds=266400; assert(album.append(r));
        } else { assert(p.depart()); assert(album.append(p)); }
        Ui::UiController view(display,sound,p,album,randomMove);
        view.setMemorialReady(true); view.init(0); uint32_t t=1500;
        auto step=[&](E e) { view.update(e,++t); view.render(); };
        step(E::None); step(E::Press);
        if(!resting) step(E::Press); // FarewellDone -> options -> categories.
        else { step(E::Up); step(E::Up); step(E::Press); } // Home -> menu -> categories.
        assert(view.screen()==S::MemorialCategories);
        if(resting) step(E::Down);
        step(E::Press); assert(view.screen()==S::Graveyard);
        char name[64]; snprintf(name,sizeof(name),"memorial-composite-%s-%s",resting?"resting":"departed",baby?"baby":"adult");
        capture(view,name);
    }
    // Full UI tests exercise reminder delivery, cancellation and visible hysteresis.
    for (bool baby : {false, true}) {
        Pet::PetData p; Storage::Memorials album;
        if (baby) { p.startNewEgg(); p.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); }
        p.setMood(80); p.setSatiety(80); p.setCleanliness(100);
        Ui::UiController view(display,sound,p,album,randomMove);
        uint32_t t=1500; view.init(0);
        auto tick=[&](uint32_t dt=1) { t+=dt; view.update(E::None,t); view.render(); };
        auto effect=[&](bool visible) {
            const int x=32+(baby?40:39), y=6+(baby?14:9);
            for(int dy=0;dy<6;++dy) for(int dx=0;dx<5;++dx) {
                const bool expected=visible && dx%2==0 && (dy<5 || dx==4);
                assert(lit(x+dx,y+dy)==expected);
            }
        };
        tick(); effect(false);
        unsigned n=careReminders;
        p.setMood(40); tick(); assert(careReminders==n+1); effect(true);
        capture(view,baby?"sad-baby-frame-0":"sad-adult-frame-0");
        tick(100); effect(true);
        capture(view,baby?"sad-baby-frame-1":"sad-adult-frame-1");
        p.setMood(44); tick(); effect(true);
        p.setMood(45); tick(); effect(false);
        p.setSatiety(25); tick(); effect(true); assert(careReminders==n+1);
        p.setSatiety(29); tick(); effect(true);
        p.setSatiety(30); tick(); effect(false);
        tick(300000); assert(careReminders==n+1); // Cancel queued reminder after care.
        p.setCleanliness(25); tick(); assert(careReminders==n+2); effect(true);
        p.setCleanliness(29); tick(); effect(true);
        p.setCleanliness(30); tick(); effect(false);
        p.setMood(40); tick(); effect(true);
        tick(300000); assert(careReminders==n+3);
        tick(300000); assert(careReminders==n+3); // No periodic nagging.
        p.setSick(true); tick(); assert(careReminders==n+4); // New illness while already sad.
        p.setSick(false); p.setMood(80); p.setCleanliness(100); tick(); effect(false);
        holdNotePlaying=true; sound.playTone(440,100); p.setMood(40); tick(300000);
        assert(careReminders==n+4); // Other sound defers the reminder.
        p.setMood(45); sound.stopTone(); holdNotePlaying=false; tick();
        assert(careReminders==n+4); // Resolved while waiting, no stale sound.
        p.setMood(40); tick(); assert(careReminders==n+5);
        assert(p.beginNormalSleep()); view.onSleepStarted(t); tick(300000);
        assert(careReminders==n+5);
        assert(p.wake()); view.onWakeSucceeded(); tick(); assert(careReminders==n+6);
        p.setMood(80); tick();
        // Unsigned elapsed time handles millis rollover.
        t=UINT32_MAX-1000; p.setMood(40); tick(); assert(careReminders==n+7);
        p.setMood(45); tick(); p.setMood(40); tick();
        tick(299999); assert(careReminders==n+8);
        // A loaded sad pet stays silent, even beyond the cooldown.
        view.init(t); unsigned loaded=careReminders; tick(1500); tick(300000);
        assert(careReminders==loaded); effect(true);
        p.setDead(true); tick(); assert(careReminders==loaded);
    }
    {
        Pet::PetData p; Storage::Memorials album;
        Ui::UiController view(display,sound,p,album,randomMove);
        uint32_t t=1500; view.init(0); view.update(E::None,t);
        unsigned n=careReminders;
        p.setMood(40); p.setSatiety(25); view.onFeedSucceeded(t);
        view.update(E::None,++t); assert(careReminders==n);
        p.setMood(45); t+=3000; view.update(E::None,t);
        assert(careReminders==n+1); // Another unmet need survives the feeding animation.
        p.setSatiety(30); view.update(E::None,++t);
        t+=300000; p.setCleanliness(25); view.onCleanSucceeded(t);
        view.update(E::None,++t); assert(careReminders==n+1);
        p.setCleanliness(30); t+=2400; view.update(E::None,t);
        view.update(E::None,++t); assert(careReminders==n+1);
        // Menu/game sounds defer a pending reminder until idle Home.
        auto step=[&](E e) { view.update(e,++t); view.render(); };
        step(E::Press); step(E::Down); step(E::Down); step(E::Down);
        step(E::Press); step(E::Press); assert(view.screen()==S::PlayCare);
        p.setMood(40); step(E::None); assert(careReminders==n+1);
        step(E::LongPress); assert(view.screen()==S::MainMenu);
        step(E::None); assert(careReminders==n+1);
        step(E::LongPress); step(E::None); assert(careReminders==n+2);
    }
    {
        Pet::PetData p; Storage::Memorials album;
        Ui::UiController view(display,sound,p,album,randomMove);
        view.init(0); view.update(E::None,1500);
        unsigned n=careReminders;
        p.setMood(40); view.update(E::None,1501); assert(careReminders==n+1);
        p.setMood(45); view.update(E::None,1502);
        p.setMood(40); view.update(E::None,1503); assert(careReminders==n+1);
#if defined(PET_SAD_TEST_MODE)
        const uint32_t cooldown=10000;
#else
        const uint32_t cooldown=300000;
#endif
        view.update(E::None,1501+cooldown-1); assert(careReminders==n+1);
        view.update(E::None,1501+cooldown); assert(careReminders==n+2);
    }
    puts("PASS: sadness thresholds/recovery, shared mark, exact cooldown/rollover, cancellation, sound/care/game deferral, sleep/wake, illness and silent boot.");
    puts("PASS: farewell confirm/cancel, healthy/sick/baby/adult frames, viewfinder/flash/postcard timing, filters, protected delete, capacity, reboot/retry and death interruption.");
    puts("PASS: real UI flow, preview glyphs/bounds, single completion, cap, replay, egg/sick/growth/death interruptions.");
}
