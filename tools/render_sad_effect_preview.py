"""Draw the shared sadness mark and preview it without changing pet art."""
from pathlib import Path
import hashlib
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ICON = ROOT / "assets/shared/effects/sad_lines.png"
OUT = ROOT / "assets/previews/pets/bird"
# Coordinates in each existing 64x44 frame, measured from the supplied references.
OFFSETS = {"baby": (40, 14), "adult": (39, 9)}


def main():
    icon = Image.new("RGBA", (5, 6))
    for x, length in ((0, 5), (2, 5), (4, 6)):
        for y in range(length):
            icon.putpixel((x, y), (255, 255, 255, 255))
    ICON.parent.mkdir(parents=True, exist_ok=True)
    icon.save(ICON)
    OUT.mkdir(parents=True, exist_ok=True)
    sheet = Image.new("RGB", (1064, 800), "#202020")
    draw = ImageDraw.Draw(sheet)
    for row, stage in enumerate(("baby", "adult")):
        source = ROOT / f"assets/pets/bird/{stage}/sad.png"
        before = hashlib.sha256(source.read_bytes()).digest()
        original = Image.open(source).convert("RGBA")
        frames = []
        for frame in range(2):
            pet = original.crop((0, frame * 44, 64, (frame + 1) * 44))
            pet.alpha_composite(icon, OFFSETS[stage])
            pet.save(OUT / f"sad_lines_{stage}_{frame}.png")
            display = Image.new("RGBA", pet.size, "black")
            display.alpha_composite(pet)
            display = display.convert("RGB").resize((512, 352), Image.Resampling.NEAREST)
            frames.append(display)
            x, y = 12 + frame * 532, 12 + row * 400
            draw.text((x, y), f"{stage.upper()} / FRAME {frame + 1} / OFFSET {OFFSETS[stage]}", fill="white")
            sheet.paste(display, (x, y + 25))
        frames[0].save(OUT / f"sad_lines_{stage}.gif", save_all=True,
                       append_images=frames[1:], duration=[1600, 1000], loop=0)
        assert hashlib.sha256(source.read_bytes()).digest() == before
    sheet.save(OUT / "sad_lines_preview.png")
    print(f"Icon: {ICON}; 16 white pixels, transparent background")
    print(f"Preview: {OUT / 'sad_lines_preview.png'}")
    print("Both original sad sprite sheets preserved byte-for-byte.")


if __name__ == "__main__":
    main()
