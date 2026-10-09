"""Convert original wyvern sheets, without changing any source pixels."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'src/ui/WyvernSprite.h'
SHEETS = [('baby', 'idle', 1, 2), ('adult', 'idle', 1, 2),
          ('baby', 'sad', 1, 2), ('adult', 'sad', 1, 2),
          ('baby', 'sleep', 1, 2), ('adult', 'sleep', 1, 1),
          ('baby', 'eating', 2, 2), ('adult', 'eating', 2, 2)]

def main():
    lines = ['#pragma once', '#include <Arduino.h>', '#include <stdint.h>',
             '// Generated from assets/pets/wyvern; zero bits are white.',
             'namespace Ui { namespace PetIcons {']
    for stage, state, cols, rows in SHEETS:
        path = ROOT / f'assets/pets/wyvern/{stage}/{state}.png'
        image = Image.open(path).convert('RGBA')
        if image.size != (cols*64, rows*44):
            raise ValueError(f'Unexpected size {image.size}: {path}')
        symbol = 'k' + ('BabyWyvern' if stage == 'baby' else 'Wyvern') + state.title() + 'Frames'
        lines += [f'// {path.relative_to(ROOT).as_posix()}',
                  f'const uint8_t {symbol}[{cols*rows}][352] PROGMEM = {{']
        for frame in range(cols*rows):
            x0, y0 = frame%cols*64, frame//cols*44
            data = []
            for y in range(44):
                for byte in range(8):
                    value = 255
                    for bit in range(8):
                        r,g,b,a = image.getpixel((x0+byte*8+bit,y0+y))
                        if a > 127:
                            if (r,g,b) != (255,255,255):
                                raise ValueError(f'Expected white or transparent pixels: {path}')
                            value &= ~(128>>bit)
                    data.append(value)
            lines.append('    {')
            for start in range(0,352,8):
                lines.append('        '+', '.join(f'0x{x:02X}' for x in data[start:start+8])+',')
            lines.append('    },')
        lines.append('};')
    lines.append('} } // namespace Ui::PetIcons')
    OUTPUT.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print('Generated 19 original 64x44 wyvern frames (6688 bytes).')

if __name__ == '__main__': main()
