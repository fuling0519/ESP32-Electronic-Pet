"""Generate the shared OLED sadness effect from the approved PNG."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
image = Image.open(ROOT / "assets/shared/effects/sad_lines.png").convert("RGBA")
assert image.size == (5, 6)
data = []
for y in range(6):
    bits = 255
    for x in range(5):
        r, g, b, a = image.getpixel((x, y))
        if a > 127:
            assert (r, g, b) == (255, 255, 255)
            bits &= ~(128 >> x)
    data.append(bits)
(ROOT / "src/ui/SadEffect.h").write_text(
    '#pragma once\n#include <stdint.h>\n'
    '// Generated from assets/shared/effects/sad_lines.png; zero bits are white.\n'
    'namespace Ui { namespace PetIcons {\n'
    'const uint8_t kSadLines[6] PROGMEM = {' + ', '.join(f'0x{v:02X}' for v in data) + '};\n'
    '} }\n', encoding='utf-8')
