"""Convert the five 21x21 potion cells to OLED bitmap data."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/shared/items/potion.png"
OUTPUT = ROOT / "src/ui/PotionSprite.h"


def main():
    image = Image.open(SOURCE).convert("RGBA")
    if image.size != (42, 63):
        raise ValueError(f"Expected a 42x63 potion sheet, got {image.size}")
    frames = []
    for x0, y0 in ((0, 0), (21, 0), (0, 21), (21, 21), (0, 42)):
        data = []
        for y in range(21):
            for byte_x in range(3):
                bits = 0xFF
                for bit in range(8):
                    x = byte_x * 8 + bit
                    if x >= 21:
                        continue
                    red, green, blue, alpha = image.getpixel((x0 + x, y0 + y))
                    if alpha > 127:
                        if (red, green, blue) != (255, 255, 255):
                            raise ValueError("Potion pixels must be white or transparent")
                        bits &= ~(0x80 >> bit)
                data.append(bits)
        frames.append("    {" + ", ".join(f"0x{b:02X}" for b in data) + "}")
    OUTPUT.write_text(
        "#pragma once\n#include <stdint.h>\n"
        "// Generated from assets/shared/items/potion.png; zero bits are white.\n"
        "namespace Ui { namespace PetIcons {\n"
        "constexpr uint8_t kPotionWidth = 21;\n"
        "constexpr uint8_t kPotionHeight = 21;\n"
        "const uint8_t kPotionFrames[5][63] PROGMEM = {\n"
        + ",\n".join(frames) + "\n};\n} } // namespace Ui::PetIcons\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
