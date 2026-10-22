"""Build the adult wyvern startled (poked) still from the generated source.

Input : web-preview/assets/pets/wyvern/adult/shock-source-v1.png (1254x1254 RGBA,
        ChatGPT web image generation with idle-preview-v1.png as reference)
Output: shock-still-v1.png / .json next to it.

Only cleans alpha and re-positions the whole character: alpha becomes binary
(the faint halo is dropped, the ~99% opaque body is made fully opaque), hidden
colour under transparent pixels is cleared, only the largest connected
character is kept, and the result is cropped with a fixed margin. Character
pixels are never scaled or redrawn. Standard library only (no Pillow).
"""
import hashlib
import json
import struct
import sys
import zlib
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIR = ROOT / "web-preview/assets/pets/wyvern/adult"
SRC, OUT = DIR / "shock-source-v1.png", DIR / "shock-still-v1"
ALPHA_CUT = 128   # below: transparent halo; at or above: solid character
MARGIN = 24       # transparent padding around the cropped character
FEET_BAND = 40    # bottom rows used to find the horizontal feet centre


def read_png(path):
    data = path.read_bytes()
    pos, idat = 8, b""
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        chunk = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
        elif kind == b"IDAT":
            idat += chunk
        pos += 12 + n
    if (depth, ctype, interlace) != (8, 6, 0):
        sys.exit(f"{path.name}: expected 8-bit non-interlaced RGBA, got depth {depth} type {ctype}")
    raw, stride, out, prev, p = zlib.decompress(idat), w * 4, [], bytearray(w * 4), 0
    for _ in range(h):
        f, line = raw[p], bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        for x in range(stride):
            a = line[x - 4] if x >= 4 else 0
            b, c = prev[x], prev[x - 4] if x >= 4 else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + b) & 255
            elif f == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        out.append(line)
        prev = line
    return w, h, out


def write_png(path, w, h, rows):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def largest_component(solid, w, h):
    seen, best = bytearray(w * h), []
    for start in range(w * h):
        if not solid[start] or seen[start]:
            continue
        seen[start], queue, comp = 1, deque([start]), []
        while queue:
            i = queue.popleft()
            comp.append(i)
            x, y = i % w, i // w
            for j in ((i - 1) if x else -1, (i + 1) if x < w - 1 else -1, i - w, i + w):
                if 0 <= j < w * h and solid[j] and not seen[j]:
                    seen[j] = 1
                    queue.append(j)
        if len(comp) > len(best):
            best = comp
    keep = bytearray(w * h)
    for i in best:
        keep[i] = 1
    return keep, len(best)


def main():
    w, h, rows = read_png(SRC)
    solid = bytearray(1 if rows[i // w][(i % w) * 4 + 3] >= ALPHA_CUT else 0 for i in range(w * h))
    keep, kept = largest_component(solid, w, h)
    dropped = sum(solid) - kept
    xs = [i % w for i in range(w * h) if keep[i]]
    ys = [i // w for i in range(w * h) if keep[i]]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    feet = [i % w for i in range(w * h) if keep[i] and i // w > y1 - FEET_BAND]
    cw, ch = x1 - x0 + 1 + 2 * MARGIN, y1 - y0 + 1 + 2 * MARGIN
    out = []
    for y in range(ch):
        line = bytearray(cw * 4)
        sy = y + y0 - MARGIN
        if 0 <= sy < h:
            for x in range(cw):
                sx = x + x0 - MARGIN
                if 0 <= sx < w and keep[sy * w + sx]:
                    line[x * 4:x * 4 + 3] = rows[sy][sx * 4:sx * 4 + 3]
                    line[x * 4 + 3] = 255
        out.append(line)
    write_png(Path(f"{OUT}.png"), cw, ch, out)
    anchor = [round((min(feet) + max(feet)) / 2) - x0 + MARGIN, y1 - y0 + MARGIN]
    meta = {
        "source": SRC.name,
        "sourceSha256": hashlib.sha256(SRC.read_bytes()).hexdigest(),
        "sourceSize": [w, h],
        "sourceCrop": [x0 - MARGIN, y0 - MARGIN, cw, ch],
        "size": [cw, ch],
        "anchor": anchor,
        "anchorNote": "feet centre x, lowest opaque row y",
        "hornToFeet": y1 - y0 + 1,
        "alphaCut": ALPHA_CUT,
        "droppedPixels": dropped,
    }
    Path(f"{OUT}.json").write_text(json.dumps(meta, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(meta))


if __name__ == "__main__":
    main()
