"""Render actual UiController cleaning framebuffers from tools/test_rps.ps1."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return ImageOps.invert(Image.open(ROOT / ".pio/rps-preview" / f"{name}.pbm").convert("L"))


def main():
    sheet = Image.new("RGB", (1596, 1200), "#202020")
    draw = ImageDraw.Draw(sheet)
    for row, stage in enumerate(("adult", "baby")):
        names = [f"clean-{stage}-before"] + [f"clean-{stage}-{i}" for i in range(4)]
        names += [f"clean-{stage}-restored"]
        frames = [read(name) for name in names]
        for i, (name, frame) in enumerate(zip(names, frames)):
            x, y = (i % 3) * 532 + 10, (row * 2 + i // 3) * 300 + 8
            draw.text((x, y), name, fill="white")
            sheet.paste(frame.resize((512, 256), Image.Resampling.NEAREST), (x, y + 22))
        large = [f.resize((512, 256), Image.Resampling.NEAREST) for f in frames]
        large[0].save(ROOT / "docs" / f"cleaning-{stage}-preview.gif", save_all=True,
                      append_images=large[1:], duration=[900, 400, 400, 400, 1200, 1600], loop=0)
    sheet.save(ROOT / "docs/cleaning-preview.png")


if __name__ == "__main__":
    main()
