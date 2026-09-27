"""Convert the user-drawn 64x88 life-stage sheets into OLED bitmap bytes."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "src" / "ui" / "BirdSprite.h"
WIDTH = 64
HEIGHT = 44
FRAME_COUNT = 2
SHEETS = (
    ("kBirdFrames", ROOT / "assets" / "BIRD" / "BIRD-normal.png"),
    ("kBabyBirdFrames", ROOT / "assets" / "BIRD" / "BIRD-baby.png"),
    ("kEggFrames", ROOT / "assets" / "EGG" / "Egg-normal.png"),
    ("kCrackedEggFrames", ROOT / "assets" / "EGG" / "Egg-born.png"),
)


def read_frames(source: Path) -> list[list[int]]:
    image = Image.open(source).convert("RGBA")
    if image.size != (WIDTH, HEIGHT * FRAME_COUNT):
        raise ValueError(f"Expected 64x88 sprite sheet, got {image.size}: {source}")

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
                            raise ValueError(f"Sprite pixels must be white or transparent: {source}")
                    else:
                        bits |= 0x80 >> bit  # Display::drawGlyph uses zero for white.
                data.append(bits)
        frames.append(data)
    return frames


def main() -> None:
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "// Generated from the 64x88 life-stage sheets in assets. Do not edit by hand.",
        "// Row-major, MSB-first; zero bits are visible white pixels.",
        "namespace Ui { namespace PetIcons {",
        "constexpr uint8_t kBirdFrameWidth = 64;",
        "constexpr uint8_t kBirdFrameHeight = 44;",
    ]
    for symbol, source in SHEETS:
        lines.append(f"// {source.relative_to(ROOT).as_posix()}")
        lines.append(f"const uint8_t {symbol}[2][352] PROGMEM = {{")
        for data in read_frames(source):
            lines.append("    {")
            for offset in range(0, len(data), 8):
                values = ", ".join(f"0x{value:02X}" for value in data[offset:offset + 8])
                lines.append(f"        {values},")
            lines.append("    },")
        lines.append("};")
    lines += ["} }  // namespace Ui::PetIcons", ""]
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(OUTPUT)


if __name__ == "__main__":
    main()
