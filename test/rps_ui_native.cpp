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
void Sound::playSuccess() {} void Sound::playFailure() {}
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
