"""Build crack stages and broken-shell pieces for the three web egg designs.

Input : web-preview/assets/pets/egg/egg-designs-v1.png / .json (the three eggs)
        web-preview/assets/pets/egg/egg-crack-hatch-source-v1.png (1536x1024,
        ChatGPT, prompt egg-crack-hatch-v1-prompt.md: one PLAIN egg as intact,
        small crack, large crack, broken bottom, top cap in a 3x2 grid)
Output: egg-hatch-v1.png / .json next to them. Rows follow the design order
        (spotted, dots, zigzag); columns are small crack, large crack, top cap,
        bottom shell. Every cell has the egg-designs cell size and anchor, and
        the two pieces stay where they were on the intact egg.

The large crack is taken as the difference between the intact and large-crack
cells of the plain egg, mapped onto each design egg's box. The small crack is
the middle of that same crack, so the crack visibly grows instead of jumping.
The pieces are cut from the real design egg along the main crack line, so the
shell pattern always matches the intact egg. The shell only parts slightly in
the web transition (the user found an open, empty inside odd), so only an
outline and a light rim are drawn along both broken edges; the GPT bottom and
cap cells are not used.
Requires only Pillow.
"""
import colorsys
import hashlib
import json
from collections import deque
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DIR = ROOT / "web-preview/assets/pets/egg"
DESIGNS, SRC, OUT = DIR / "egg-designs-v1", DIR / "egg-crack-hatch-source-v1.png", DIR / "egg-hatch-v1"
ALPHA_CUT = 128
DIFF_CUT = 40          # RGB difference that counts as crack in the plain egg
EDGE_MARGIN = 14       # ignore differences this close to the plain egg's outline
SMALL_SPAN = 0.30      # small crack = this share of the egg width around the middle
LINE = 12              # broken-edge outline thickness, same as the design outline
RIM = 7                # light shell-thickness rim along broken edges
CRACK_DARK = (74, 38, 24)


def hsv(p):
    return colorsys.rgb_to_hsv(p[0] / 255, p[1] / 255, p[2] / 255)


def nb(x, y):
    return ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1))


def solid_box(im, box):
    cell = im.crop(box)
    x0, y0, x1, y1 = cell.getchannel("A").point(lambda v: 255 if v >= ALPHA_CUT else 0).getbbox()
    return cell.crop((x0, y0, x1, y1))


def plain_cells():
    src = Image.open(SRC).convert("RGBA")
    w, h = src.size
    cells = [solid_box(src, (c * w // 3, r * h // 2, (c + 1) * w // 3, (r + 1) * h // 2))
             for r in range(2) for c in range(3) if (r, c) != (1, 2)]
    return cells  # intact, small, large, bottom, cap


def crack_mask(intact, cracked):
    """Crack pixels of the plain egg as {(x, y): 'dark' | 'light'} in plain-egg coordinates."""
    a, b = intact.load(), cracked.load()
    inside = {(x, y) for y in range(intact.height) for x in range(intact.width) if a[x, y][3] >= ALPHA_CUT}
    edge = {p for p in inside if any(n not in inside for n in nb(*p))}
    for _ in range(EDGE_MARGIN):
        edge |= {n for p in edge for n in nb(*p) if n in inside}
    out = {}
    for p in inside - edge:
        if max(abs(a[p][k] - b[p][k]) for k in range(3)) > DIFF_CUT:
            out[p] = "dark" if hsv(b[p])[2] < 0.5 else "light"
    # Keep only crack-sized groups (drops stray highlight jitter).
    seen, keep = set(), {}
    for p in out:
        if p in seen:
            continue
        comp, q = [p], deque([p])
        seen.add(p)
        while q:
            for n in nb(*q.popleft()):
                if n in out and n not in seen:
                    seen.add(n)
                    comp.append(n)
                    q.append(n)
        if len(comp) > 150:
            keep.update({c: out[c] for c in comp})
    return keep


def map_mask(mask, src_size, dst_box):
    """Nearest-neighbour map of plain-egg crack pixels onto a design egg box."""
    sw, sh = src_size
    x0, y0, x1, y1 = dst_box
    dw, dh = x1 - x0, y1 - y0
    out = {}
    for y in range(dh):
        for x in range(dw):
            v = mask.get((int(x * sw / dw), int(y * sh / dh)))
            if v:
                out[(x0 + x, y0 + y)] = v
    return out


def main_line(mask):
    """y of the main crack for every x, following the dark pixels from left to right and skipping branches."""
    dark = {}
    for (x, y), v in mask.items():
        if v == "dark":
            dark.setdefault(x, []).append(y)
    xs = sorted(dark)
    line, prev = {}, None
    for x in xs:
        runs, ys = [], sorted(dark[x])
        for y in ys:
            if runs and y == runs[-1][1] + 1:
                runs[-1][1] = y
            else:
                runs.append([y, y])
        if prev is None:  # branches never reach the outline, so the first column is the main line
            a, b = runs[0]
            prev = (a + b) / 2
        else:
            a, b = min(runs, key=lambda r: 0 if r[0] <= prev <= r[1] else min(abs(r[0] - prev), abs(r[1] - prev)))
            if min(abs(a - prev), abs(b - prev)) > LINE and not a <= prev <= b:
                line[x] = prev  # gap or a lone branch column: hold the line
                continue
            # A long vertical run is the line plus a branch: stay as close to the previous height as it allows.
            prev = min(max(prev, a + LINE / 2), b - LINE / 2) if b - a >= LINE else (a + b) / 2
        line[x] = prev
    return line


def shell_colour(egg_px, box):
    """Median light shell colour of the upper egg (ignores outline, glint and pattern by brightness/saturation rank)."""
    x0, y0, x1, y1 = box
    vals = []
    for y in range(y0 + (y1 - y0) // 8, y0 + (y1 - y0) // 3):
        for x in range(x0 + (x1 - x0) // 3, x0 + 2 * (x1 - x0) // 3):
            p = egg_px[x, y]
            if p[3] and 0.07 < hsv(p)[1] < 0.3 and hsv(p)[2] > 0.85:
                vals.append(p[:3])
    vals.sort(key=lambda c: sum(c))
    return vals[len(vals) // 2]


def build_row(cell_x, cell_w, cell_h, egg_px, box, large_src, src_size):
    large = map_mask(large_src, src_size, box)
    x0, y0, x1, y1 = box
    mid = (x0 + x1) / 2
    half = (x1 - x0) * SMALL_SPAN / 2
    small_raw = {p: v for p, v in large.items() if abs(p[0] - mid) <= half}
    # Largest connected piece of the middle window.
    seen, best = set(), []
    for p in small_raw:
        if p in seen:
            continue
        comp, q = [p], deque([p])
        seen.add(p)
        while q:
            for n in nb(*q.popleft()):
                if n in small_raw and n not in seen:
                    seen.add(n)
                    comp.append(n)
                    q.append(n)
        best = max(best, comp, key=len)
    small = {p: small_raw[p] for p in best}

    egg = {(x, y): egg_px[x, y] for y in range(cell_h) for x in range(cell_x, cell_x + cell_w) if egg_px[x, y][3]}

    def cracked(mask):
        out = dict(egg)
        for p, v in mask.items():
            if p not in out:
                continue
            if v == "dark":
                out[p] = CRACK_DARK + (255,)
            else:
                o = out[p]
                out[p] = tuple(round(c + (255 - c) * 0.55) for c in o[:3]) + (255,)
        return out

    line = main_line(large)
    xs = sorted(line)
    # Extend the line flat to the egg's outline at both ends.
    for x in range(x0, x1):
        if x not in line:
            line[x] = line[xs[0]] if x < xs[0] else line[xs[-1]]
    top_edge = {x: line[x] - LINE / 2 for x in line}
    bot_edge = {x: line[x] + LINE / 2 for x in line}

    light = tuple(round(c + (255 - c) * 0.6) for c in shell_colour(egg_px, box))

    cap, bottom = {}, {}
    outline_dark = CRACK_DARK + (255,)
    for (x, y), p in egg.items():
        lx = line.get(x)
        if lx is None:
            continue
        if y < top_edge[x]:
            cap[(x, y)] = p
        elif y < top_edge[x] + LINE:
            cap[(x, y)] = outline_dark
        if y >= bot_edge[x]:
            bottom[(x, y)] = p
        elif y >= bot_edge[x] - LINE:
            bottom[(x, y)] = outline_dark
    # The shell only parts slightly (no open inside is ever shown), so both broken
    # edges are just an outline plus a light shell-thickness rim next to it.
    for x in line:
        rims = ((cap, range(round(top_edge[x]) - RIM, round(top_edge[x]))),
                (bottom, range(round(bot_edge[x]), round(bot_edge[x]) + RIM)))
        for piece, ys in rims:
            for y in ys:
                p = piece.get((x, y))
                if p and p != outline_dark and hsv(p)[2] >= 0.45:
                    piece[(x, y)] = light + (255,)
    return [cracked(small), cracked(large), cap, bottom], line


def main():
    designs = Image.open(DESIGNS.with_suffix(".png")).convert("RGBA")
    meta = json.loads(DESIGNS.with_suffix(".json").read_text(encoding="utf-8"))
    cell_w, cell_h = meta["cell"]
    order = meta["order"]
    intact, _small, large_src_img, _bottom, _cap = plain_cells()
    large_src = crack_mask(intact, large_src_img)

    dp = designs.load()
    sheet = Image.new("RGBA", (cell_w * 4, cell_h * len(order)), (0, 0, 0, 0))
    spx = sheet.load()
    lines = {}
    for row, name in enumerate(order):
        cx0 = row * cell_w
        box_img = designs.crop((cx0, 0, cx0 + cell_w, cell_h))
        bx0, by0, bx1, by1 = box_img.getchannel("A").getbbox()
        box = (cx0 + bx0, by0, cx0 + bx1, by1)
        cells, line = build_row(cx0, cell_w, cell_h, dp, box, large_src, intact.size)
        for col, pixels in enumerate(cells):
            for (x, y), p in pixels.items():
                spx[col * cell_w + (x - cx0), row * cell_h + y] = p
        lines[name] = [[x - cx0, round(line[x])] for x in sorted(line)[::4]]
    sheet.save(OUT.with_suffix(".png"))
    out = {"designs": DESIGNS.name + ".png", "source": SRC.name,
           "sourceSha256": hashlib.sha256(SRC.read_bytes()).hexdigest(),
           "rows": order, "columns": ["smallCrack", "largeCrack", "topCap", "bottomShell"],
           "cell": [cell_w, cell_h], "anchor": meta["anchor"], "crackLine": lines}
    OUT.with_suffix(".json").write_text(json.dumps(out, ensure_ascii=False), encoding="utf-8")
    # Same data as a script so file:// pages can read it without fetch.
    OUT.with_name("egg-hatch-data-v1.js").write_text(
        "window.EggHatchData=" + json.dumps(out, ensure_ascii=False, separators=(",", ":")) + ";\n", encoding="utf-8")
    print(f"sheet {sheet.size}")


if __name__ == "__main__":
    main()
