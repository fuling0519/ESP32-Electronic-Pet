"""Phase 1 only: real Home framebuffer + proposed local eye/effect pixels.

No production UI, canonical PNG, sprite header or input code is modified.
Requires tools/test_rps.ps1 first (native U8g2 objects), g++, Pillow.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / '.pio/touch-proposal'
OUT = ROOT / 'docs/previews/touch-proposal'
WORK.mkdir(parents=True, exist_ok=True)
OUT.mkdir(parents=True, exist_ok=True)
ROLES = [
    ('bird-baby', '幼鳥', 'BirdSprite.h', 'kBabyBirdFrames', True, False,
     [(24, 25, 29, 29), (35, 25, 40, 29)], (44, 17), [(62, 11, 7)]),
    ('bird-adult', '成鳥', 'BirdSprite.h', 'kBirdFrames', False, False,
     [(19, 18, 25, 25), (31, 20, 38, 26)], (91, 14), [(65, 2, 7)]),
    ('wyvern-baby', '幼龍', 'WyvernSprite.h', 'kBabyWyvernIdleFrames', True, True,
     [(24, 25, 29, 30), (35, 25, 40, 30)], (44, 17), [(59, 10, 7), (73, 14, 5)]),
    ('wyvern-adult', '成龍', 'WyvernSprite.h', 'kWyvernIdleFrames', False, True,
     [(23, 20, 29, 26), (37, 20, 43, 26)], (96, 14), [(96, 6, 7)]),
]
HEARTS = {7: ['0110110', '1111111', '1111111', '0111110', '0011100', '0001000', '0000000'],
          5: ['01010', '11111', '11111', '01110', '00100']}
EXCLAIM = ['111', '111', '111', '010', '010', '000', '010']


def read_sprite(file, symbol):
    source = (ROOT / 'src/ui' / file).read_text(encoding='utf-8')
    body = re.search(r'const uint8_t ' + symbol + r'\[.*?=\s*\{(.*?)\n\};', source, re.S).group(1)
    values = [int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{2})', body)][:352]
    im = Image.new('L', (64, 44))
    im.putdata([255 if not values[y*8+x//8] & (128 >> (x % 8)) else 0
                for y in range(44) for x in range(64)])
    return im


def expression(role, kind, phase):
    _, _, file, symbol, baby, dragon, patches, _, _ = role
    original = read_sprite(file, symbol)
    im = original.copy()
    draw = ImageDraw.Draw(im)
    for eye, (l, t, r, b) in enumerate(patches):
        draw.rectangle((l, t, r-1, b-1), fill=255)
        cx = (l+r-1)//2
        cy = (t+b-1)//2
        if kind == 'petting':
            # Smile arches (^), deliberately different from sleeping lids (v).
            draw.line([(cx-2, cy+1), (cx-1, cy), (cx+1, cy), (cx+2, cy+1)], fill=0)
        elif baby:
            draw.ellipse((l, t, r-1, b-1), fill=0)
            draw.point((cx, t+1), fill=255)
            if phase:
                draw.point((cx+1, t+1), fill=255)
        elif dragon:
            # Restrained questioning squint; horns, mouth and body stay intact.
            draw.line([(l, cy), (r-1, cy+1)], fill=0)
            if phase == 0:
                draw.line([(l+1, t), (r-2, t)], fill=0)
        else:
            # Adult bird: one quick blink, then a questioning half-open eye.
            if phase == 0:
                draw.line([(l, cy), (r-2, cy+1)], fill=0)
            else:
                im.paste(original.crop((l, t, r, b)), (l, t))
    # Enforce that proposed art differs only inside the enumerated eye patches.
    allowed = Image.new('L', im.size)
    ad = ImageDraw.Draw(allowed)
    for l, t, r, b in patches:
        ad.rectangle((l, t, r-1, b-1), fill=255)
    assert not ImageChops.subtract(ImageChops.difference(im, original), allowed).getbbox()
    return im


def bitmap(im):
    data = []
    for y in range(44):
        for byte in range(8):
            bits = 255
            for b in range(8):
                if im.getpixel((byte*8+b, y)):
                    bits &= ~(128 >> b)
            data.append(bits)
    return ','.join(str(x) for x in data)


def glyph(pattern, x, y):
    return '\n'.join(f'display.drawLine({x+c},{y+r},{x+c},{y+r});'
                     for r, row in enumerate(pattern) for c, bit in enumerate(row) if bit == '1')


def offset(role, kind, phase):
    baby, dragon = role[4:6]
    if kind == 'petting':
        return (0, 1 if dragon and not baby else phase)
    if baby:
        return ((phase if dragon else 0), -2 if phase == 0 else -1)
    return ((1 if phase == 0 else 0), 0)


# The unchanged test adapter gives exactly the firmware's U8g2 fonts and glyphs.
reference = (ROOT / 'test/rps_ui_native.cpp').read_text(encoding='utf-8')
adapter = reference.split('static void capture(')[0]
source = adapter + '\nstatic void saveFrame(const char* name) {\n'
source += 'char path[200]; snprintf(path,sizeof(path),".pio/touch-proposal/%s.pbm",name);\n'
source += 'FILE* f=fopen(path,"wb"); assert(f); fprintf(f,"P1\\n128 64\\n");\n'
source += 'for(int y=0;y<64;++y){for(int x=0;x<128;++x) fprintf(f,"%u ",(pixels[(y/8)*128+x]>>(y%8))&1); fputc(10,f);} fclose(f);}\n'
source += '''int main() {
info.tile_width=16; info.tile_height=8; info.pixel_width=128; info.pixel_height=64;
gfx.u8x8.display_info=&info;
u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);
u8g2_SetDrawColor(&gfx,1);
'''
for role in ROLES:
    key, _, _, _, baby, dragon, patches, bang, hearts = role
    stage = 'Baby' if baby else 'Adult'
    species = 'Wyvern' if dragon else 'Bird'
    source += '{ Hardware::Display display; Hardware::Sound sound; Pet::PetData pet; Storage::Memorials album;\n'
    source += f'assert(pet.startNewEgg(1,"Tamama",Pet::SpeciesId::{species}));\n'
    source += f'auto s=pet.snapshot(); s.lifeStage=Pet::LifeStage::{stage}; s.ageSeconds=Pet::PetData::k{"EggHatch" if baby else "Adult"}AgeSeconds; assert(pet.restore(s));\n'
    source += 'Ui::UiController ui(display,sound,pet,album,randomMove); ui.init(0); ui.update(Hardware::InputEvent::None,2000); ui.update(Hardware::InputEvent::None,2600); ui.render();\n'
    # Capture the unmodified UI, including both status-focus states.
    source += f'saveFrame("{key}-base"); auto base=pixels;\n'
    source += f'ui.update(Hardware::InputEvent::Right,2601); ui.render(); saveFrame("{key}-focus"); pixels=base;\n'
    for kind in ('poke', 'petting'):
        for phase in (0, 1):
            name = f'{key}-{kind}-{phase}'
            dx, dy = offset(role, kind, phase)
            proposed = expression(role, kind, phase)
            source += '{ pixels=base; display.clearArea(32,4,64,48);\n'
            source += f'static const uint8_t art[352]={{{bitmap(proposed)}}};\n'
            source += f'display.drawGlyph({32+dx},{6+dy},art,64,44);\n'
            if kind == 'poke':
                source += glyph(EXCLAIM, *bang)
            else:
                for hx, hy, size in hearts:
                    source += glyph(HEARTS[size], hx, hy-phase)
            source += f'\nsaveFrame("{name}"); }}\n'
    source += '}\n'
source += '}\n'
(WORK / 'preview.cpp').write_text(source, encoding='utf-8')
clib = ROOT / '.pio/libdeps/esp32dev/U8g2/src/clib'
objects = list((ROOT / '.pio/rps-preview').glob('u8*.o'))
assert objects, 'Run tools/test_rps.ps1 first.'
subprocess.run(['g++', '-std=c++11', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                '-Itest/support', '-Isrc', '-Iinclude', '-I'+str(clib), str(WORK/'preview.cpp'),
                'src/ui/UiController.cpp', 'src/ui/PetIcons.cpp', 'src/games/RpsGame.cpp',
                'src/games/MemoryNotes.cpp', 'src/pet/PetData.cpp', 'src/pet/PetSnapshot.cpp',
                'src/storage/Memorials.cpp', *map(str, objects), '-o', str(WORK/'preview.exe')], cwd=ROOT, check=True)
subprocess.run([str(WORK/'preview.exe')], cwd=ROOT, check=True)


def load(name):
    # P1 black means set bit. OLED set bit must be white.
    im = Image.open(WORK / (name+'.pbm')).convert('L')
    return ImageChops.invert(im)


def lit(im):
    return {(x, y) for y in range(im.height) for x in range(im.width) if im.getpixel((x, y))}


validation = {'status': 'PROPOSAL_ONLY_NOT_FIRMWARE_OR_HARDWARE_ACCEPTANCE', 'roles': []}
font = ImageFont.truetype('C:/Windows/Fonts/msjh.ttc', 20)
sheet = Image.new('RGB', (1080, 4*310+58), '#171b21')
sd = ImageDraw.Draw(sheet)
sd.text((16, 10), '觸摸構圖提案｜左：戳戳　右：摸摸頭｜真實 Home 底圖＋提案眼部像素', font=font, fill='white')
for row, role in enumerate(ROLES):
    key, label, file, symbol, baby, dragon, patches, bang, hearts = role
    base = load(key+'-base')
    base.save(OUT/(key+'-base-128x64.png'))
    focus = load(key+'-focus')
    assert not any(base.getpixel((x,y)) != focus.getpixel((x,y))
                   for y in range(64) for x in range(109)), 'Focus unexpectedly changed central artwork'
    entry = {'role': key, 'sprite_origin': [32, 6], 'eye_patches_local_exclusive': patches,
             'exclamation': [*bang, 3, 7], 'hearts': hearts, 'frames': []}
    for col, kind in enumerate(('poke', 'petting')):
        for phase in (0, 1):
            name = f'{key}-{kind}-{phase}'
            im = load(name)
            assert im.size == (128, 64) and set(im.tobytes()) <= {0, 255}
            dx, dy = offset(role, kind, phase)
            sprite = expression(role, kind, phase)
            sprite_points = {(x+32+dx,y+6+dy) for x,y in lit(sprite)}
            effects = set()
            marks = [(EXCLAIM, *bang)] if kind == 'poke' else [(HEARTS[n],x,y-phase) for x,y,n in hearts]
            for pattern, x0, y0 in marks:
                effects |= {(x0+x,y0+y) for y,r in enumerate(pattern) for x,c in enumerate(r) if c == '1'}
            assert not sprite_points & effects, name+' overlaps character'
            min_gap = min(max(abs(x-a),abs(y-b)) for x,y in effects for a,b in sprite_points)-1
            assert min_gap >= 2, (name, min_gap)
            assert all(20 <= x <= 108 and 0 <= y <= 54 for x,y in sprite_points|effects)
            # Reserved HUD and footer: exact equality, including dark pixels.
            for y in range(64):
                for x in range(128):
                    if x < 20 or x >= 109 or y >= 55:
                        assert im.getpixel((x,y)) == base.getpixel((x,y)), (name,x,y)
            # The character area contains exactly the original silhouette + eye edits.
            assert lit(im) & {(x,y) for y in range(4,52) for x in range(32,96)} == (sprite_points|effects) & {(x,y) for y in range(4,52) for x in range(32,96)}
            im.save(OUT/(name+'-128x64.png'))
            im.resize((512,256), Image.Resampling.NEAREST).save(OUT/(name+'-512x256.png'))
            # The only focused-status pixels lie in the protected HUD; compose
            # that real focus variant and check it remains intact as well.
            focused = im.copy()
            focused.paste(focus.crop((109,0,128,55)), (109,0))
            assert focused.crop((109,0,128,55)).tobytes() == focus.crop((109,0,128,55)).tobytes()
            entry['frames'].append({'name':name, 'offset':[dx,dy], 'effect_clear_blank_pixels':min_gap,
                                    'hud_footer_unchanged':True, 'sha256':hashlib.sha256(im.tobytes()).hexdigest()})
        x, y = 16+col*536, 58+row*310
        sd.text((x,y), label+'｜'+('戳戳・500ms' if kind=='poke' else '摸頭・閉眼笑弧'), font=font, fill='white')
        sheet.paste(load(f'{key}-{kind}-0').convert('RGB').resize((512,256), Image.Resampling.NEAREST),(x,y+32))
    # Complete illustrative timelines. These simulate the proposed UI timing only.
    poke_names = ['base','poke-0','poke-1','base']
    pet_names = ['base','petting-0','petting-1','petting-0','petting-1','petting-0','base']
    for kind, names, durations in [('poke',poke_names,[700,160,340,700]),
                                  ('petting',pet_names,[700,400,400,400,400,250,700])]:
        frames = [load(f'{key}-{name}') for name in names]
        for scale in (1,4):
            enlarged=[f.resize((128*scale,64*scale),Image.Resampling.NEAREST) for f in frames]
            enlarged[0].save(OUT/f'{key}-{kind}-{128*scale}x{64*scale}.gif', save_all=True,
                             append_images=enlarged[1:], duration=durations, loop=0, disposal=2)
            gif = Image.open(OUT/f'{key}-{kind}-{128*scale}x{64*scale}.gif')
            assert gif.n_frames == len(enlarged)
            for i, expected in enumerate(enlarged):
                gif.seek(i)
                assert gif.convert('L').tobytes() == expected.tobytes()
                assert gif.info['duration'] == durations[i]
    validation['roles'].append(entry)
sheet.save(OUT/'contact-sheet.png')
# Inspect the second reaction frame at the same scale before delivery.
second = sheet.copy()
for row, role in enumerate(ROLES):
    for col, kind in enumerate(('poke','petting')):
        second.paste(load(f'{role[0]}-{kind}-1').convert('RGB').resize((512,256),Image.Resampling.NEAREST),
                     (16+col*536,58+row*310+32))
second.save(OUT/'contact-sheet-phase-1.png')
validation['source_sha256'] = {p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in
    ['src/ui/UiController.cpp','src/ui/PetIcons.cpp','src/ui/BirdSprite.h','src/ui/WyvernSprite.h','test/rps_ui_native.cpp']}
(OUT/'validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: 16 proposed native frames, eye-only artwork edits, effects clearance >=2px, HUD/footer unchanged.')
print(OUT/'contact-sheet.png')
