"""Preview only: two sound modes, using project glyphs and U8g2 fonts."""
from pathlib import Path
import re
import subprocess
from PIL import Image, ImageDraw, ImageOps
import generate_ui_font as font

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '.pio/sound-toggle-preview'
OUT.mkdir(parents=True, exist_ok=True)
font.CHARACTERS = ''.join(dict.fromkeys('選單紀念冊送別聲音靜有左右切換'))
font.write_header(OUT / 'PreviewFont.h', font.parse_bdf(
    ROOT / '.pio/status-font-tools/wenquanyi_9pt.bdf'))

# Reuse only the host renderer's text/base/capture helpers, never its main.
reference = (ROOT / 'tools/render_volume_preview.py').read_text(encoding='utf-8')
helpers = re.search(r"source = r'''(.*?)int main\(\)", reference, re.S).group(1)
helpers = helpers.replace('.pio/volume-preview/', '.pio/sound-toggle-preview/')
source = helpers + r'''
int main() {
    info.tile_width=16; info.tile_height=8; info.pixel_width=128; info.pixel_height=64;
    gfx.u8x8.display_info=&info;
    u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);
    u8g2_SetDrawColor(&gfx,1);
    base("選單"); text(99,14,"3/3");
    text(22,32,"紀念冊"); text(22,46,"送別"); text(22,60,"聲音");
    u8g2_SetFont(&gfx,u8g2_font_5x7_tf); u8g2_DrawStr(&gfx,8,60,">");
    capture("menu");
    for(int enabled=0;enabled<2;++enabled) {
        pixels.fill(0);
        // Both states occupy the same centered 32 x 32 icon area.
        u8g2_DrawFrame(&gfx,48,21,8,12);
        u8g2_DrawLine(&gfx,55,21,65,11);
        u8g2_DrawLine(&gfx,65,11,65,42);
        u8g2_DrawLine(&gfx,65,42,55,32);
        if(!enabled) u8g2_DrawLine(&gfx,48,42,79,11);
        else {
            u8g2_DrawLine(&gfx,69,20,73,24);
            u8g2_DrawLine(&gfx,73,24,73,29);
            u8g2_DrawLine(&gfx,73,29,69,33);
            u8g2_DrawLine(&gfx,73,15,79,21);
            u8g2_DrawLine(&gfx,79,21,79,32);
            u8g2_DrawLine(&gfx,79,32,73,38);
        }
        text(40,58,"左右切換");
        capture(enabled?"on":"off");
    }
}
'''
(OUT / 'preview.cpp').write_text('#include <cstdlib>\n' + source, encoding='utf-8')
clib = ROOT / '.pio/libdeps/esp32dev/U8g2/src/clib'
objects = [str(p) for p in (ROOT / '.pio/rps-preview').glob('u8*.o')]
assert objects, 'Existing native U8g2 renderer objects are required.'
subprocess.run(['g++', '-std=c++11', '-ffunction-sections', '-fdata-sections',
    '-Wl,--gc-sections', '-Itest/support', '-I'+str(clib), '-I'+str(OUT),
    str(OUT / 'preview.cpp'), *objects, '-o', str(OUT / 'preview.exe')], cwd=ROOT, check=True)
subprocess.run([str(OUT / 'preview.exe')], cwd=ROOT, check=True)
sheet = Image.new('RGB', (800, 232), '#1c1e22')
draw = ImageDraw.Draw(sheet)
for name in ('menu', 'off', 'on'):
    frame = ImageOps.invert(Image.open(OUT / (name+'.pbm')).convert('L'))
    assert frame.size == (128, 64)
    assert set(frame.tobytes()) <= {0,255}
    frame.save(ROOT / 'docs' / ('sound-toggle-'+name+'-proposal.png'))
    if name == 'menu':
        continue
    x = (0 if name == 'off' else 1)*400+8
    draw.text((x,8), name.upper()+' / 128x64 PROPOSAL', fill='white')
    sheet.paste(frame.resize((384,192), Image.Resampling.NEAREST), (x,28))
sheet.save(ROOT / 'docs/sound-toggle-proposal.png')
print('Created two-mode sound proposal; firmware source and settings unchanged.')
