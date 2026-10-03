"""Convert the user-drawn 64x88 life-stage sheets into OLED bitmap bytes."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "src" / "ui" / "BirdSprite.h"
WIDTH = 64
HEIGHT = 44
FRAME_COUNT = 2
SHEETS = (
    ("kBirdFrames", ROOT / "assets" / "pets" / "bird" / "adult" / "idle.png"),
    ("kBabyBirdFrames", ROOT / "assets" / "pets" / "bird" / "baby" / "idle.png"),
    ("kEggFrames", ROOT / "assets" / "shared" / "egg" / "idle.png"),
    ("kCrackedEggFrames", ROOT / "assets" / "shared" / "egg" / "hatching.png"),
    ("kBirdEatingFrames", ROOT / "assets" / "pets" / "bird" / "adult" / "eating.png"),
    ("kBabyBirdEatingFrames", ROOT / "assets" / "pets" / "bird" / "baby" / "eating.png"),
    ("kBirdSadFrames", ROOT / "assets" / "pets" / "bird" / "adult" / "sad.png"),
    ("kBabyBirdSadFrames", ROOT / "assets" / "pets" / "bird" / "baby" / "sad.png"),
)


def close_eyes(image: Image.Image, symbol: str, frame: int) -> None:
    """Replace only the open eyes in the new bird artwork."""
    shift = frame  # Both second frames sit one pixel lower.
    if symbol == "kBirdSleepingFrames":
        patches = ((19, 18, 25, 25), (31, 20, 38, 26))
        eyelids = (((19, 21), (20, 22), (21, 22), (22, 22), (23, 21)),
                   ((32, 23), (33, 24), (34, 24), (35, 24), (36, 23)))
    else:
        patches = ((24, 25, 29, 29), (35, 25, 40, 29))
        eyelids = (((24, 26), (25, 27), (26, 27), (27, 27), (28, 26)),
                   ((35, 26), (36, 27), (37, 27), (38, 27), (39, 26)))

    for left, top, right, bottom in patches:
        for y in range(top + shift, bottom + shift):
            for x in range(left, right):
                image.putpixel((x, y), (255, 255, 255, 255))
    for eyelid in eyelids:
        for x, y in eyelid:
            image.putpixel((x, y + shift), (0, 0, 0, 0))


def read_frames(source: Path, symbol: str, width=WIDTH, height=HEIGHT,
                count=FRAME_COUNT) -> list[list[int]]:
    image = Image.open(source).convert("RGBA")
    if image.size != (width, height * count):
        raise ValueError(f"Unexpected sprite sheet size {image.size}: {source}")

    frames = []
    for frame in range(count):
        frame_image = image.crop((0, frame * height, width, (frame + 1) * height))
        if symbol in ("kBirdSleepingFrames", "kBabyBirdSleepingFrames"):
            close_eyes(frame_image, symbol, frame)
        data = []
        for y in range(height):
            for byte_x in range((width + 7) // 8):
                bits = 0
                for bit in range(8):
                    if byte_x * 8 + bit >= width:
                        bits |= 0x80 >> bit
                        continue
                    red, green, blue, alpha = frame_image.getpixel((byte_x * 8 + bit, y))
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
    output_sheets = SHEETS[:2] + (
        ("kBirdSleepingFrames", SHEETS[0][1]),
        ("kBabyBirdSleepingFrames", SHEETS[1][1]),
    ) + SHEETS[2:]
    for symbol, source in output_sheets:
        lines.append(f"// {source.relative_to(ROOT).as_posix()}")
        lines.append(f"const uint8_t {symbol}[2][352] PROGMEM = {{")
        for data in read_frames(source, symbol):
            lines.append("    {")
            for offset in range(0, len(data), 8):
                values = ", ".join(f"0x{value:02X}" for value in data[offset:offset + 8])
                lines.append(f"        {values},")
            lines.append("    },")
        lines.append("};")
    lines += ["// assets/shared/items/food_bowl.png: three 21x15 frames, full to empty.",
              "constexpr uint8_t kBowlWidth = 21;",
              "constexpr uint8_t kBowlHeight = 15;",
              "const uint8_t kBowlFrames[3][45] PROGMEM = {"]
    for data in read_frames(ROOT / "assets/shared/items/food_bowl.png", "kBowlFrames", 21, 15, 3):
        lines.append("    {" + ", ".join(f"0x{value:02X}" for value in data) + "},")
    lines += ["};", "} }  // namespace Ui::PetIcons", ""]
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(OUTPUT)


if __name__ == "__main__":
    main()
