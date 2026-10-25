"""Draw the white bird's happy face (closed smiling eyes) onto the approved idle image.

Usage: python tools/draw_white_bird_happy.py
Input:  web-preview/assets/pets/bird/white-idle-v2.png (1254 px square)
Output: web-preview/assets/pets/bird/white-happy-v1.png (same canvas, same body position)

Only the two eyes change: each is painted over with the surrounding face colour and
redrawn as a tilted upward arc on the art's own ~8.7 px pixel grid, so the new marks match
its pixel size. The blush, beak, wings and tail are left untouched. Requires only Pillow.
"""
import math
from pathlib import Path

from PIL import Image

DIR = Path(__file__).resolve().parents[1] / "web-preview/assets/pets/bird"
CELL = 1254 / 144  # the source art's pixel grid
# Eye boxes (left, top, right, bottom) measured from the dark eye pixels of white-idle-v2.png.
EYES = [(326, 299, 492, 452), (632, 433, 792, 572)]
PAD = 14
LINE = (58, 33, 26, 255)  # the eyes' own dark outline brown
# Clockwise tilt per eye (left, right), so the eyes follow the face's three-quarter turn; the far (left)
# eye was turned back a little by the user.
TILTS = [28, 36]
# Per-eye nudge in whole grid cells after centring, fitted to the user's hand-placed left eye.
NUDGE = [(-2, -2), (0, 0)]
LIFT = 0.05  # the closed eye sits slightly above the open eye's centre, as a fraction of its height


def is_pink(c):
    return c[0] > 215 and c[1] < 205 and c[0] - c[2] > 25


def plain(c):
    """A light face pixel: not blush, outline or transparent."""
    return c[3] > 240 and min(c[:3]) > 215 and not is_pink(c)


def erase(px, box):
    """Fill the eye with the face's own shading: per pixel, blend the ellipse's edge colours
    across the row and the column, then average per grid cell so it keeps the pixel look."""
    l, t, r, b = box
    cx, cy, rx, ry = (l + r) / 2, (t + b) / 2, (r - l) / 2 + PAD, (b - t) / 2 + PAD
    inside = lambda x, y: ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1
    fill = {}
    for y in range(int(cy - ry), int(cy + ry) + 1):
        for x in range(int(cx - rx), int(cx + rx) + 1):
            if not inside(x, y) or is_pink(px[x, y]):
                continue
            x0, x1, y0, y1 = x, x, y, y
            while inside(x0, y):
                x0 -= 1
            while inside(x1, y):
                x1 += 1
            while inside(x, y0):
                y0 -= 1
            while inside(x, y1):
                y1 += 1
            u, v = (x - x0) / (x1 - x0), (y - y0) / (y1 - y0)
            # Blush or outline at an edge would streak across the face; such a direction is skipped.
            blends = [[a[i] * (1 - k) + c[i] * k for i in range(3)]
                      for a, c, k in ((px[x0, y], px[x1, y], u), (px[x, y0], px[x, y1], v)) if plain(a) and plain(c)]
            fill[x, y] = [sum(c[i] for c in blends) / len(blends) for i in range(3)] if blends else None
    known = [c for c in fill.values() if c]
    face = [sum(c[i] for c in known) / len(known) for i in range(3)]
    cells = {}
    for (x, y), c in fill.items():
        cells.setdefault((int(x / CELL), int(y / CELL)), []).append(c or face)
    for (x, y) in fill:
        group = cells[int(x / CELL), int(y / CELL)]
        px[x, y] = tuple(round(sum(c[i] for c in group) / len(group)) for i in range(3)) + (255,)


def eye_centre(px, box):
    """Centroid of the open eye's dark pixels, so the closed eye keeps its spacing to the beak."""
    l, t, r, b = box
    pts = [(x, y) for y in range(t, b + 1) for x in range(l, r + 1) if px[x, y][3] > 200 and sum(px[x, y][:3]) < 330]
    return sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts)


def arc_cells(box, centre, tilt):
    """Grid cells of an upward arc (a closed, smiling eye) over the upper part of the eye.
    The arc is built around the open eye's centroid, then shifted by whole cells so its
    bounding box is centred on it (a little higher), keeping the eye's spacing to the beak."""
    l, t, r, b = box
    ex, ey = centre
    rx, ry = (r - l) * 0.43, (b - t) * 0.3
    cos, sin = math.cos(math.radians(tilt)), math.sin(math.radians(tilt))
    cells = []
    for gy in range(int(t / CELL) - 4, int(b / CELL) + 4):
        for gx in range(int(l / CELL) - 4, int(r / CELL) + 4):
            # Rotate into the arc's own frame: tilted clockwise so the outer (right) end hooks down.
            ox, oy = (gx + 0.5) * CELL - ex, (gy + 0.5) * CELL - ey
            x, y = ox * cos + oy * sin, -ox * sin + oy * cos
            # A band about two cells thick; the ends stop a little above the arc's centre line.
            if abs((x / rx) ** 2 + (y / ry) ** 2 - 1) <= 0.5 and y <= -0.1 * ry:
                cells.append((gx, gy))
    xs = [(gx + 0.5) * CELL for gx, _ in cells]
    ys = [(gy + 0.5) * CELL for _, gy in cells]
    dx = round((ex - (min(xs) + max(xs)) / 2) / CELL)
    dy = round((ey - (b - t) * LIFT - (min(ys) + max(ys)) / 2) / CELL)
    return [(gx + dx, gy + dy) for gx, gy in cells]


def main():
    im = Image.open(DIR / "white-idle-v2.png").convert("RGBA")
    px = im.load()
    centres = [eye_centre(px, box) for box in EYES]
    for box, centre, tilt, (nx, ny) in zip(EYES, centres, TILTS, NUDGE):
        erase(px, box)
        for gx, gy in arc_cells(box, centre, tilt):
            gx, gy = gx + nx, gy + ny
            for y in range(round(gy * CELL), round((gy + 1) * CELL)):
                for x in range(round(gx * CELL), round((gx + 1) * CELL)):
                    px[x, y] = LINE
    im.save(DIR / "white-happy-v1.png")
    print("wrote", DIR / "white-happy-v1.png")


if __name__ == "__main__":
    main()
