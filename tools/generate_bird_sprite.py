"""Convert the user-drawn 64x88 idle sheet into OLED bitmap bytes."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "BIRD" / "BIRD-normal.png"
OUTPUT = ROOT / "src" / "ui" / "BirdSprite.h"
WIDTH = 64
HEIGHT = 44
FRAME_COUNT = 2


def main() -> None:
    image = Image.open(SOURCE).convert("RGBA")
    if image.size != (WIDTH, HEIGHT * FRAME_COUNT):
        raise ValueError(f"Expected 64x88 sprite sheet, got {image.size}")

    frames = []
    for frame in range(FRAME_COUNT):
        data = []
        for y in range(frame * HEIGHT, (frame + 1) * HEIGHT):
            for byte_x in range(WIDTH // 8):
                bits = 0
                for bit in range(8):
                    red, green, blue, alpha = image.getpixel((byte_x * 8 + bit, y))
                    if alpha > 127:
                        if (red, green, blue) != (255, 255, 255):
                            raise ValueError("Sprite pixels must be white or transparent")
                    else:
                        bits |= 0x80 >> bit  # Display::drawGlyph uses zero for white.
                data.append(bits)
        frames.append(data)

    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "// Generated from assets/BIRD/BIRD-normal.png. Do not edit by hand.",
        "// Row-major, MSB-first; zero bits are visible white pixels.",
        "namespace Ui { namespace PetIcons {",
        "constexpr uint8_t kBirdFrameWidth = 64;",
        "constexpr uint8_t kBirdFrameHeight = 44;",
        "const uint8_t kBirdFrames[2][352] PROGMEM = {",
    ]
    for data in frames:
        lines.append("    {")
        for offset in range(0, len(data), 8):
            values = ", ".join(f"0x{value:02X}" for value in data[offset:offset + 8])
            lines.append(f"        {values},")
        lines.append("    },")
    lines += ["};", "} }  // namespace Ui::PetIcons", ""]
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(OUTPUT)


if __name__ == "__main__":
    main()
