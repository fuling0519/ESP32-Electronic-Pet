"""Render treatment previews from real UI framebuffers emitted by test_rps.ps1."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "docs"
FRAMEBUFFER = ROOT / ".pio/rps-preview"


def read(name):
    # PBM black bits represent lit OLED pixels in the native test emitter.
    return ImageOps.invert(Image.open(FRAMEBUFFER / (name + ".pbm")).convert("L"))


def main():
    sheet = Image.new("RGB", (3 * 532, 4 * 300), "#202020")
    draw = ImageDraw.Draw(sheet)
    for species_index, stage in enumerate(("adult", "baby")):
        names = [f"treat-{stage}-pour-{i}" for i in range(4)]
        names += [f"treat-{stage}-empty", f"treat-{stage}-sparkle-3"]
        for i, name in enumerate(names):
            x = (i % 3) * 532 + 10
            y = (species_index * 2 + i // 3) * 300 + 8
            draw.text((x, y), name, fill="white")
            sheet.paste(read(name).resize((512, 256), Image.Resampling.NEAREST), (x, y + 22))
        names = [f"treat-{stage}-pour-{i}" for i in range(4)]
        names += [f"treat-{stage}-empty"]
        names += [f"treat-{stage}-sparkle-{i}" for i in range(16)]
        frames = [read(name).resize((512, 256), Image.Resampling.NEAREST) for name in names]
        frames[0].save(OUTPUT / f"treatment-{stage}-preview.gif", save_all=True,
                       append_images=frames[1:], duration=[250, 250, 350, 350, 200] + [125] * 16,
                       loop=0)
    sheet.save(OUTPUT / "treatment-preview.png")


if __name__ == "__main__":
    main()
