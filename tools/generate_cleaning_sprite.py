"""Convert the confirmed four cleaning cells to 84x49 OLED bitmap frames."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT = 84, 49  # Exactly 6/7 of each 98x57 source cell.


def main():
    image = Image.open(ROOT / "assets/shared/effects/cleaning.png").convert("RGBA")
    if image.size != (196, 114):
        raise ValueError(f"Expected a 196x114 cleaning sheet, got {image.size}")
    background = Image.new("RGBA", image.size, "black")
    background.alpha_composite(image)
    pixels = background.convert("L").point(lambda p: 255 if p >= 128 else 0)
    frames = []
    stride = (WIDTH + 7) // 8
    for x, y in ((0, 0), (98, 0), (0, 57), (98, 57)):
        frame = pixels.crop((x, y, x + 98, y + 57)).resize(
            (WIDTH, HEIGHT), Image.Resampling.NEAREST)
        data = []
        for row in range(HEIGHT):
            for byte_x in range(stride):
                bits = 0xFF
                for bit in range(8):
                    col = byte_x * 8 + bit
                    if col < WIDTH and frame.getpixel((col, row)):
                        bits &= ~(0x80 >> bit)
                data.append(bits)
        frames.append("    {" + ", ".join(f"0x{b:02X}" for b in data) + "}")
    (ROOT / "src/ui/CleaningSprite.h").write_text(
        "#pragma once\n#include <stdint.h>\n"
        "// Generated from assets/shared/effects/cleaning.png; zero bits are white.\n"
        "namespace Ui { namespace PetIcons {\n"
        f"constexpr uint8_t kCleaningWidth = {WIDTH};\n"
        f"constexpr uint8_t kCleaningHeight = {HEIGHT};\n"
        f"const uint8_t kCleaningFrames[4][{stride * HEIGHT}] PROGMEM = {{\n"
        + ",\n".join(frames) + "\n};\n} } // namespace Ui::PetIcons\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
