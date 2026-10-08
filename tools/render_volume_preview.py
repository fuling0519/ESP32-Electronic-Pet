"""Preview-only OLED proposal; does not edit or build ESP32 firmware.

Uses the project's Chinese bitmap source and native U8g2 ASCII renderer.
"""
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw, ImageOps
import generate_ui_font as font

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '.pio/volume-preview'
OUT.mkdir(parents=True, exist_ok=True)
font.CHARACTERS = ''.join(dict.fromkeys(
    '選單紀念冊送別音量靜小中大聲左右調整短按確定長取消保存失敗重試'))
font.write_header(OUT / 'PreviewFont.h', font.parse_bdf(
    ROOT / '.pio/status-font-tools/wenquanyi_9pt.bdf'))

source = r'''
#include <u8g2.h>
#include <array>
#include <cstdio>
#include "PreviewFont.h"
static u8g2_t gfx{};
static u8x8_display_info_t info{};
static std::array<uint8_t,1024> pixels{};
static unsigned decode(const char*& s) {
    unsigned c=(unsigned char)*s++; if(c<128) return c;
    unsigned b=(unsigned char)*s++, a=(unsigned char)*s++;
    return ((c&15)<<12)|((b&63)<<6)|(a&63);
}
static void text(int x,int y,const char* s) {
    while(*s) {
        unsigned cp=decode(s);
        if(cp<128) {
            char ascii[2]={(char)cp,0};
            u8g2_SetFont(&gfx,u8g2_font_6x10_tf);
            u8g2_DrawStr(&gfx,x,y,ascii); x+=6;
        } else {
            const uint8_t* glyph=nullptr;
            for(unsigned i=0;i<UiFont12::kGlyphCount;++i)
                if(UiFont12::kGlyphs[i].codepoint==cp) glyph=UiFont12::kGlyphs[i].bitmap;
            if(!glyph) { fprintf(stderr,"Missing glyph %u\n",cp); exit(1); }
            for(int row=0;row<12;++row) for(int col=0;col<12;++col)
                if(!(glyph[row*2+col/8]&(128>>(col%8))))
                    u8g2_DrawPixel(&gfx,x+col,y-11+row);
            x+=12;
        }
    }
}
static void base(const char* title) {
    pixels.fill(0); u8g2_DrawFrame(&gfx,0,0,128,64);
    text(9,14,title); u8g2_DrawHLine(&gfx,6,18,116);
}
static void capture(const char* name) {
    char path[120]; snprintf(path,sizeof(path),".pio/volume-preview/%s.pbm",name);
    FILE* f=fopen(path,"wb"); fprintf(f,"P1\n128 64\n");
    for(int y=0;y<64;++y) {
        for(int x=0;x<128;++x) fprintf(f,"%d ",(pixels[(y/8)*128+x]>>(y%8))&1);
        fprintf(f,"\n");
    } fclose(f);
}
int main() {
    info.tile_width=16; info.tile_height=8; info.pixel_width=128; info.pixel_height=64;
    gfx.u8x8.display_info=&info;
    u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);
    u8g2_SetDrawColor(&gfx,1);
    const char* menu[]={"紀念冊","送別","音量"};
    base("選單"); text(99,14,"3/3");
    for(int row=0;row<3;++row) text(22,32+row*14,menu[row]);
    u8g2_SetFont(&gfx,u8g2_font_5x7_tf); u8g2_DrawStr(&gfx,8,60,">");
    capture("menu");
    const char* levels[]={"靜音","小聲","中聲","大聲"};
    const char* names[]={"mute","low","medium","high"};
    for(int level=0;level<4;++level) {
        base("音量"); char page[4]; snprintf(page,4,"%d/4",level+1); text(99,14,page);
        // Pixel speaker: body, flared cone, and two sound waves.
        u8g2_DrawFrame(&gfx,12,31,5,8);
        u8g2_DrawLine(&gfx,16,31,23,25);
        u8g2_DrawLine(&gfx,23,25,23,44);
        u8g2_DrawLine(&gfx,23,44,16,38);
        if(level==0) {
            u8g2_DrawLine(&gfx,10,44,29,25);
        } else {
            u8g2_DrawLine(&gfx,27,30,30,33);
            u8g2_DrawLine(&gfx,30,33,30,36);
            u8g2_DrawLine(&gfx,30,36,27,39);
            u8g2_DrawLine(&gfx,31,27,35,31);
            u8g2_DrawLine(&gfx,35,31,35,38);
            u8g2_DrawLine(&gfx,35,38,31,42);
        }
        // Display levels, independent of the underlying PWM duty.
        u8g2_DrawFrame(&gfx,45,29,72,12);
        if(level) u8g2_DrawBox(&gfx,47,31,68*level/3,8);
        text(32,57,"<"); text(52,57,levels[level]); text(90,57,">");
        capture(names[level]);
    }
}
'''
(OUT / 'preview.cpp').write_text('#include <cstdlib>\n' + source, encoding='utf-8')
clib = ROOT / '.pio/libdeps/esp32dev/U8g2/src/clib'
objects = [str(p) for p in (ROOT / '.pio/rps-preview').glob('u8*.o')]
assert objects, 'Existing native U8g2 objects are required.'
subprocess.run(['g++', '-std=c++11', '-ffunction-sections', '-fdata-sections',
    '-Wl,--gc-sections', '-Itest/support', '-I'+str(clib), '-I'+str(OUT),
    str(OUT / 'preview.cpp'), *objects, '-o', str(OUT / 'preview.exe')],
    cwd=ROOT, check=True)
subprocess.run([str(OUT / 'preview.exe')], cwd=ROOT, check=True)
names = ['menu','mute','low','medium','high']
labels = ['MENU / PAGE 3', 'MUTE / LEVEL 0', 'LOW / LEVEL 1',
          'MEDIUM / LEVEL 2', 'HIGH / LEVEL 3']
sheet = Image.new('RGB',(1048,900),'#1c1e22')
draw = ImageDraw.Draw(sheet)
for i,(name,label) in enumerate(zip(names,labels)):
    frame = ImageOps.invert(Image.open(OUT / (name+'.pbm')).convert('L'))
    assert frame.size==(128,64)
    frame.save(ROOT / 'docs' / ('volume-'+name+'-proposal.png'))
    x,y=8+(i%2)*524,12+(i//2)*300
    draw.text((x,y),label,fill='white')
    sheet.paste(frame.resize((512,256),Image.Resampling.NEAREST),(x,y+24))
draw.text((532,636),'128 x 64 / PREVIEW ONLY',fill='white')
draw.text((532,660),'LEFT/RIGHT: SELECT + AUDITION',fill='white')
draw.text((532,684),'SPEAKER + BAR / MUTE SLASH',fill='white')
sheet.save(ROOT / 'docs/volume-ui-proposal.png')
print('Created menu + four volume proposals using project bitmap fonts.')
