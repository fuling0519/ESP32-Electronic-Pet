"""Cut a standing white-bird pose (sleep, startled) out of its green-screen ChatGPT source.

Usage: python tools/prepare_white_bird_pose.py sleep|startled
Input:  web-preview/assets/pets/bird/white-<pose>-source-v1.png (1254 px square, like white-idle-v2.png)
Output: white-<pose>-v1.png (cropped, transparent) and white-<pose>-data-v1.js / .json

Same keying as tools/prepare_white_bird_dead.py; pixels are never scaled or redrawn.
The anchor is the feet: the centre of the dark-grey toe pixels (tail excluded) and the
lowest toe row, measured the same way on white-idle-v2.png, so the web can plant the
new pose exactly where the approved base image stands.
ChatGPT drew both birds smaller than the idle one, so each also gets a "scale" applied
around the toes on the web: startled matches the idle white-body height; the roosting
sleep pose sits lower on purpose, so it matches the idle white-feather area instead.
"""
import json
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from prepare_white_bird_dead import DIR, drop_specks, key_pixel  # noqa: E402

TAIL_X = 0.72  # fraction of the canvas; the tail sits right of this, the feet left of it
MATCH = {"startled": "height", "sleep": "area"}  # what each pose matches on the idle image


def white_height(im):
    """Height of the light body feathers (top of the head to the lowest white row)."""
    w, h = im.size
    px, rows = im.load(), []
    for y in range(h):
        for x in range(0, w, 2):
            r, g, b, a = px[x, y]
            if a > 200 and min(r, g, b) > 200:
                rows.append(y)
                break
    return rows[-1] - rows[0]


def white_area(im):
    """Count of light body-feather pixels."""
    return sum(1 for r, g, b, a in im.getdata() if a > 200 and min(r, g, b) > 200)


def feet(im):
    """Centre x and bottom y of the grey toes in the bottom band of the figure."""
    w, h = im.size
    px, bottom = im.load(), im.getbbox()[3]
    xs, low = [], 0
    for y in range(bottom - 90, bottom):
        for x in range(int(w * TAIL_X)):
            r, g, b, a = px[x, y]
            if a and max(r, g, b) - min(r, g, b) < 24 and 40 < r < 150:
                xs.append(x)
                low = max(low, y + 1)
    return sum(xs) / len(xs), low


def main(pose):
    rgb = Image.open(DIR / f"white-{pose}-source-v1.png").convert("RGB")
    out = Image.new("RGBA", rgb.size)
    raw = rgb.tobytes()
    out.putdata([key_pixel(*raw[i:i + 3]) for i in range(0, len(raw), 3)])
    drop_specks(out)
    box = out.getbbox()
    fx, fy = feet(out)
    base = Image.open(DIR / "white-idle-v2.png").convert("RGBA")
    bx, by = feet(base)
    scale = round(white_height(base) / white_height(out) if MATCH[pose] == "height" else (white_area(base) / white_area(out)) ** .5, 3)
    out.crop(box).save(DIR / f"white-{pose}-v1.png")
    w, h = box[2] - box[0], box[3] - box[1]
    meta = {"source": list(rgb.size), "crop": {"left": box[0], "top": box[1], "width": w, "height": h},
            "feet": [round(fx - box[0], 1), fy - box[1]], "baseFeet": [round(bx, 1), by], "scale": scale}
    name = "White" + pose.capitalize()
    (DIR / f"white-{pose}-data-v1.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
    (DIR / f"white-{pose}-data-v1.js").write_text(f"window.WhiteBird{name[5:]}Data=" + json.dumps(meta, separators=(",", ":")) + ";\n", encoding="utf-8")
    print(pose, json.dumps(meta))


if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.argv[1] not in ("sleep", "startled"):
        sys.exit(__doc__)
    main(sys.argv[1])
