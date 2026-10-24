"""Cut the white bird's lying-down (dead) pose out of its green-screen ChatGPT source.

Usage: python tools/prepare_white_bird_dead.py
Input:  web-preview/assets/pets/bird/white-dead-source-v1.png (1254 px square, like white-idle-v2.png)
Output: white-dead-v1.png (cropped, transparent) and white-dead-data-v1.js / .json

The source was drawn on the same 1254 px canvas as the approved idle image, so the
web keeps one source pixel = one idle source pixel; pixels are never scaled or
redrawn here. Small specks (stray keyed noise) are dropped; separate body parts
such as feet are kept. Requires only Pillow.
"""
import json
from collections import deque
from pathlib import Path

from PIL import Image

DIR = Path(__file__).resolve().parents[1] / "web-preview/assets/pets/bird"
MIN_PART = 200  # pixels; anything smaller is keying noise
BODY_CENTRE = 0.42  # fraction of the crop width where the round body is centred


def key_pixel(r, g, b):
    """Green screen to crisp alpha; de-green kept edge pixels (no halo)."""
    if g - max(r, b) > 45:
        return (0, 0, 0, 0)
    return (r, min(g, max(r, b) + 10), b, 255)


def drop_specks(im):
    w, h = im.size
    px, seen = im.load(), set()
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
                if len(comp) < MIN_PART:
                    for p in comp:
                        px[p] = (0, 0, 0, 0)


def main():
    rgb = Image.open(DIR / "white-dead-source-v1.png").convert("RGB")
    out = Image.new("RGBA", rgb.size)
    raw = rgb.tobytes()
    out.putdata([key_pixel(*raw[i:i + 3]) for i in range(0, len(raw), 3)])
    drop_specks(out)
    box = out.getbbox()
    out.crop(box).save(DIR / "white-dead-v1.png")
    w, h = box[2] - box[0], box[3] - box[1]
    # Anchor: body centre (the tail lies off to the right, so not the crop centre) on the ground line.
    meta = {"source": list(rgb.size), "crop": {"left": box[0], "top": box[1], "width": w, "height": h}, "anchor": [round(w * BODY_CENTRE), h]}
    (DIR / "white-dead-data-v1.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
    (DIR / "white-dead-data-v1.js").write_text("window.WhiteBirdDeadData=" + json.dumps(meta, separators=(",", ":")) + ";\n", encoding="utf-8")
    print(json.dumps(meta))


if __name__ == "__main__":
    main()
