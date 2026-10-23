"""Build a clean baby wyvern sheet from a green-screen ChatGPT grid.

Usage: python tools/prepare_wyvern_baby_sheet.py <source.png> <cols> <rows> <out-name> [keep]
  keep: optional comma list of source cell indices to export (others are rejected art)
Output: web-preview/assets/pets/wyvern/baby/<out-name>.png / .json

Removes the background, lightens only the lavender skin by SKIN_LIGHTEN (the
user picked 25% on 2026-10-23; outline, eyes, blush and cream stay as drawn),
keeps each cell's largest connected character, and re-positions whole frames
onto a shared square cell with one feet line. Frames are centred on the head
band so wings and tail swinging do not shift the body. Character pixels are
never scaled or redrawn. Requires only Pillow.
"""
import colorsys
import hashlib
import json
import sys
from collections import deque
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "web-preview/assets/pets/wyvern/baby"
SKIN_LIGHTEN = 0.25


def key_pixel(r, g, b):
    """Green screen to crisp alpha; de-green kept edge pixels (no halo)."""
    if g - max(r, b) > 45:
        return (0, 0, 0, 0)
    return (r, min(g, max(r, b) + 10), b, 255)


def lighten_skin(px):
    r, g, b, a = px
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    if l > 0.45 and 0.62 < h < 0.85:  # lavender skin and its light shading only
        l, s = l + (1 - l) * SKIN_LIGHTEN, s * (1 - SKIN_LIGHTEN * 0.5)
        r, g, b = (round(v * 255) for v in colorsys.hls_to_rgb(h, l, s))
    return (r, g, b, a)


def largest_component(im):
    w, h = im.size
    px, seen, best = im.load(), set(), []
    for y in range(h):
        for x in range(w):
            if px[x, y][3] and (x, y) not in seen:
                comp, q = [], deque([(x, y)])
                seen.add((x, y))
                while q:
                    cx, cy = q.popleft()
                    comp.append((cx, cy))
                    for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                        if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in seen and px[nx, ny][3]:
                            seen.add((nx, ny))
                            q.append((nx, ny))
                if len(comp) > len(best):
                    best = comp
    out = Image.new("RGBA", im.size, (0, 0, 0, 0))
    op = out.load()
    for x, y in best:
        op[x, y] = lighten_skin(px[x, y])
    return out.crop(out.getbbox())


def head_centre(im):
    """Horizontal centre of the head band (15-40% down), symmetric in every pose."""
    band = im.crop((0, int(im.height * .15), im.width, int(im.height * .40)))
    x0, _, x1, _ = band.getbbox()
    return (x0 + x1 - 1) / 2


def main():
    src, cols, rows, name = Path(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    keep = [int(i) for i in sys.argv[5].split(",")] if len(sys.argv) > 5 else list(range(cols * rows))
    rgb = Image.open(src).convert("RGB")
    w, h = rgb.size
    keyed, rp = Image.new("RGBA", rgb.size), rgb.load()
    kp = keyed.load()
    for y in range(h):
        for x in range(w):
            kp[x, y] = key_pixel(*rp[x, y])
    cells = []
    for i in keep:
        row, col = divmod(i, cols)
        c = largest_component(keyed.crop((col * w // cols, row * h // rows,
                                          (col + 1) * w // cols, (row + 1) * h // rows)))
        cells.append((c, head_centre(c)))
    half = max(max(cx, c.width - cx) for c, cx in cells)
    cell = int(max(2 * half, max(c.height for c, _ in cells)) * 1.12)
    feet, centre = cell - int(cell * .05), cell // 2
    out_cols = min(cols, len(cells))
    out_rows = -(-len(cells) // out_cols)
    sheet = Image.new("RGBA", (cell * out_cols, cell * out_rows), (0, 0, 0, 0))
    for i, (c, cx) in enumerate(cells):
        sheet.alpha_composite(c, ((i % out_cols) * cell + round(centre - cx), (i // out_cols) * cell + feet - c.height))
    sheet.save(OUT_DIR / f"{name}.png")
    meta = {"source": src.name, "sourceSha256": hashlib.sha256(src.read_bytes()).hexdigest(),
            "sourceSize": [w, h], "sourceGrid": [cols, rows], "sourceCells": keep,
            "grid": [out_cols, out_rows], "cell": cell, "anchor": [centre, feet],
            "skinLighten": SKIN_LIGHTEN, "order": "left-right,top-bottom",
            "heights": [c.height for c, _ in cells]}
    (OUT_DIR / f"{name}.json").write_text(json.dumps(meta, ensure_ascii=False, indent=1), encoding="utf-8")
    print(f"cell {cell}, anchor {centre},{feet}, heights {meta['heights']}")


if __name__ == "__main__":
    main()
