"""Build the three web egg designs from the ChatGPT three-egg source.

Input : web-preview/assets/pets/egg/egg-designs-source-v1.png (1536x1024 RGBA,
        ChatGPT web image generation, prompt egg-designs-v1-prompt.md)
Output: egg-designs-v1.png / .json next to it; cells left to right are
        spotted, dots, zigzag.

Alpha becomes binary (the invisible dark glow is dropped, the ~99% opaque egg is
made fully opaque) and each egg is re-positioned onto a shared cell with one
bottom line. Egg pixels are never scaled or redrawn; the dots and zigzag eggs
are only recoloured as the user picked on 2026-10-24: dots egg gets a mint
shell with #f6afda / #9ce0ea dots, zigzag egg a pale-yellow shell with a sky
blue band. GPT shading, outline and glint are kept. Requires only Pillow.
"""
import colorsys
import hashlib
import json
from collections import deque
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DIR = ROOT / "web-preview/assets/pets/egg"
SRC, OUT = DIR / "egg-designs-source-v1.png", DIR / "egg-designs-v1"
ALPHA_CUT = 128
ORDER = ["spotted", "dots", "zigzag"]
DOTS_SHELL_HUE = 160                       # mint
DOT_COLORS = {"pink": (0xf6, 0xaf, 0xda), "green": (0x9c, 0xe0, 0xea)}
ZIGZAG_SHELL_HUE = 50                      # pale yellow
ZIGZAG_BAND = (120, 185, 240)              # sky blue


def hsv(p):
    return colorsys.rgb_to_hsv(p[0] / 255, p[1] / 255, p[2] / 255)


def rgb(h, s, v):
    return tuple(round(c * 255) for c in colorsys.hsv_to_rgb(h, s, v))


def is_outline(p):
    return hsv(p)[2] < 0.45


def is_glint(p):
    return hsv(p)[1] < 0.07


def nb(x, y):
    return ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1))


def components(points):
    seen, out = set(), []
    for p in points:
        if p in seen:
            continue
        comp, q = [p], deque([p])
        seen.add(p)
        while q:
            for n in nb(*q.popleft()):
                if n in points and n not in seen:
                    seen.add(n)
                    comp.append(n)
                    q.append(n)
        out.append(comp)
    return out


def split_eggs(src):
    """Binary alpha, then the largest opaque component in each third of the image."""
    w, h = src.size
    sp = src.load()
    eggs = []
    for i in range(3):
        x0, x1 = i * w // 3, (i + 1) * w // 3
        solid = {(x, y) for y in range(h) for x in range(x0, x1) if sp[x, y][3] >= ALPHA_CUT}
        best = max(components(solid), key=len)
        egg = Image.new("RGBA", src.size, (0, 0, 0, 0))
        ep = egg.load()
        for x, y in best:
            ep[x, y] = sp[x, y][:3] + (255,)
        eggs.append(egg.crop(egg.getbbox()))
    return eggs


def shell_tint(p, hue):
    """Same brightness and saturation, new hue; white glints stay white."""
    if is_glint(p):
        return p[:3]
    _, s, v = hsv(p)
    return rgb(hue / 360, min(1, s * 1.05), v)


def recolor_dots(egg):
    px = egg.load()
    orig = {(x, y): px[x, y] for y in range(egg.height) for x in range(egg.width)}
    inside = {p for p, c in orig.items() if c[3] and not is_outline(c)}

    def is_dot(c):
        h, s, v = hsv(c)
        h *= 360
        return v > 0.55 and ((h >= 330 or h <= 12) and s > 0.18 or 55 <= h <= 140 and s > 0.1)

    for p in inside:
        px[p] = shell_tint(orig[p], DOTS_SHELL_HUE) + (255,)
    for dot in components({p for p in inside if is_dot(orig[p])}):
        core = set(dot)
        edge, ring = set(), set(core)
        for step in range(6):
            ring = {n for p in ring for n in nb(*p) if n in inside} - core - edge
            if step < 2:
                edge |= ring
        shell = [orig[p] for p in ring if not is_dot(orig[p])]
        if not shell:
            continue
        local = tuple(sorted(c[k] for c in shell)[len(shell) // 2] for k in range(3))
        dot_c = tuple(sorted(orig[p][k] for p in dot)[len(dot) // 2] for k in range(3))
        h = hsv(dot_c)[0] * 360
        shade = min(1.0, hsv(local)[2] / 0.97)
        target = tuple(c * shade for c in DOT_COLORS["pink" if h >= 330 or h <= 12 else "green"])
        new_local = shell_tint(local, DOTS_SHELL_HUE)
        span = [d - s for d, s in zip(dot_c, local)]
        norm = sum(c * c for c in span) or 1
        # Anti-aliased edge pixels keep their mix ratio between shell and dot.
        for p in core | edge:
            o = orig[p]
            f = 1.0 if p in core else max(0.0, min(1.0, sum((o[k] - local[k]) * span[k] for k in range(3)) / norm))
            px[p] = tuple(round(new_local[k] + f * (target[k] - new_local[k])) for k in range(3)) + (255,)
    return egg


def band_weights(egg):
    """0 = shell, 1 = band. Shell shadows share the band hue, so compare against the
    shell saturation expected at the pixel's brightness, then median-smooth the map."""
    px = egg.load()
    raw = {}
    for y in range(egg.height):
        for x in range(egg.width):
            p = px[x, y]
            if not p[3] or is_outline(p):
                continue
            h, s, v = hsv(p)
            shell_s = 0.17 + max(0.0, 0.97 - v) * 0.75
            raw[(x, y)] = 0.0 if not 10 <= h * 360 <= 30 else max(0.0, min(1.0, (s - shell_s - 0.04) / (0.48 - shell_s)))
    smooth = {}
    for (x, y), f in raw.items():
        vals = sorted(raw.get((x + dx, y + dy), f) for dx in (-2, -1, 0, 1, 2) for dy in (-2, -1, 0, 1, 2))
        smooth[(x, y)] = vals[len(vals) // 2]
    return smooth


def recolor_zigzag(egg):
    px = egg.load()
    weights = band_weights(egg)
    for (x, y), f in weights.items():
            p = px[x, y]
            v = hsv(p)[2]
            shell = p[:3] if is_glint(p) and not f else rgb(ZIGZAG_SHELL_HUE / 360, 0.17, v)
            band = tuple(c * min(1.0, v / 0.97) for c in ZIGZAG_BAND)
            px[x, y] = tuple(round(shell[k] + f * (band[k] - shell[k])) for k in range(3)) + (255,)
    return egg


def main():
    src = Image.open(SRC).convert("RGBA")
    spotted, dots, zigzag = split_eggs(src)
    eggs = [spotted, recolor_dots(dots), recolor_zigzag(zigzag)]
    cell_w = round(max(e.width for e in eggs) * 1.1)
    cell_h = round(max(e.height for e in eggs) * 1.1)
    bottom, centre = cell_h - (cell_h - max(e.height for e in eggs)) // 2, cell_w // 2
    sheet = Image.new("RGBA", (cell_w * len(eggs), cell_h), (0, 0, 0, 0))
    for i, e in enumerate(eggs):
        sheet.alpha_composite(e, (i * cell_w + centre - e.width // 2, bottom - e.height))
    sheet.save(OUT.with_suffix(".png"))
    meta = {"source": SRC.name, "sourceSha256": hashlib.sha256(SRC.read_bytes()).hexdigest(),
            "sourceSize": list(src.size), "order": ORDER, "cell": [cell_w, cell_h],
            "anchor": [centre, bottom], "sizes": [list(e.size) for e in eggs],
            "dotsShellHue": DOTS_SHELL_HUE, "dotColors": {k: "#%02x%02x%02x" % v for k, v in DOT_COLORS.items()},
            "zigzagShellHue": ZIGZAG_SHELL_HUE, "zigzagBand": "#%02x%02x%02x" % ZIGZAG_BAND}
    OUT.with_suffix(".json").write_text(json.dumps(meta, ensure_ascii=False, indent=1), encoding="utf-8")
    print(f"cell {cell_w}x{cell_h}, anchor {centre},{bottom}, sizes {meta['sizes']}")


if __name__ == "__main__":
    main()
