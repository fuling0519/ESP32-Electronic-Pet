"""Build the adult wyvern eating sheet from the green-screen source.

Input : web-preview/assets/pets/wyvern/adult/eat-sheet-v2.png (2x2, RGB green screen)
Output: eat-sheet-v2-clean.png / .json next to it.

Only removes the background and re-positions whole frames: green spill becomes
alpha, edge pixels are de-greened, each cell keeps its largest connected
character (dropping stray marks), and frames share one feet line and centre.
Character pixels are never scaled or redrawn. Requires numpy, scipy, Pillow.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

ROOT = Path(__file__).resolve().parents[1]
DIR = ROOT / "web-preview/assets/pets/wyvern/adult"
SRC, OUT = DIR / "eat-sheet-v2.png", DIR / "eat-sheet-v2-clean"


def key_out(rgb):
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    alpha = np.clip(1 - (g - np.maximum(r, b)) / 90.0, 0, 1)
    g = np.where(alpha < 1, np.minimum(g, np.maximum(r, b)), g)
    return np.dstack([np.clip(np.stack([r, g, b], -1), 0, 255), alpha * 255]).astype(np.uint8)


def main():
    rgba = key_out(np.asarray(Image.open(SRC).convert("RGB")).astype(np.float32))
    h, w = rgba.shape[:2]
    cells = []
    for row in range(2):
        for col in range(2):
            c = rgba[row * h // 2:(row + 1) * h // 2, col * w // 2:(col + 1) * w // 2].copy()
            solid = c[..., 3] > 128
            lab, n = ndi.label(solid)
            if n:
                biggest = np.argmax(ndi.sum(solid, lab, range(1, n + 1))) + 1
                keep = ndi.binary_dilation(lab == biggest, iterations=6) & (c[..., 3] > 0)
                c[..., 3] = np.where(keep, c[..., 3], 0)
            ys, xs = np.where(c[..., 3] > 0)
            cells.append(c[ys.min():ys.max() + 1, xs.min():xs.max() + 1])
    cell = int(max(max(c.shape[0], c.shape[1]) for c in cells) * 1.25)
    feet = cell - int(cell * 0.06)
    sheet = Image.new("RGBA", (cell * 2, cell * 2), (0, 0, 0, 0))
    frames = []
    for i, c in enumerate(cells):
        im = Image.fromarray(c, "RGBA")
        frame = Image.new("RGBA", (cell, cell), (0, 0, 0, 0))
        frame.alpha_composite(im, ((cell - im.width) // 2, feet - im.height))
        # Matches the approved 2026-10-22 output: pasting with the frame as its own
        # mask slightly thins semi-transparent edge alpha (a*a/255).
        sheet.paste(frame, ((i % 2) * cell, (i // 2) * cell), frame)
        frames.append({"index": i, "feetY": feet, "scale": 1.0})
    sheet.save(f"{OUT}.png")
    Path(f"{OUT}.json").write_text(json.dumps(
        {"cell": cell, "frames": frames, "order": "left-right,top-bottom", "sourceSize": [w, h]}, indent=1))
    print(f"cell {cell}, feet y {feet}")


if __name__ == "__main__":
    main()
