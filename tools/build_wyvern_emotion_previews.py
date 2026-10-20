"""Slice the six generated emotion poses and package lossless preview assets.

Generated margins are uneven, so locate the six opaque connected silhouettes
instead of assuming the source can be divided into equal rectangular cells.
Only crop, integer translation and transparent padding are applied.
"""
import hashlib
import json
import struct
from pathlib import Path

from build_wyvern_idle_preview import SIGNATURE, chunk, chunks, compressed, decode

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'web-preview/assets/pets/wyvern/adult'
SOURCE = ASSETS / 'emotions-source-v1.png'
GROUPS = [('sad', [2200, 240]), ('sick', [1400, 900]), ('happy', [1800, 650])]


def silhouettes(width, height, pixels):
    mask = bytearray(a >= 160 for a in pixels[3::4])
    boxes = []
    for seed in range(width * height):
        if not mask[seed]:
            continue
        mask[seed], stack = 0, [seed]
        count, x0, y0, x1, y1 = 0, width, height, 0, 0
        while stack:
            n = stack.pop()
            x, y = n % width, n // width
            count += 1
            x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
            adjacent = []
            if x:
                adjacent.append(n - 1)
            if x < width - 1:
                adjacent.append(n + 1)
            if y:
                adjacent.append(n - width)
            if y < height - 1:
                adjacent.append(n + width)
            for other in adjacent:
                if mask[other]:
                    mask[other] = 0
                    stack.append(other)
        if count > 2000:
            boxes.append([x0, y0, x1, y1])
    if len(boxes) != 6:
        raise ValueError(f'Expected six separate dragon silhouettes, found {len(boxes)}')
    boxes.sort(key=lambda b: b[1] + b[3])
    return [b for row in range(3)
            for b in sorted(boxes[row * 2:row * 2 + 2], key=lambda b: b[0])]


def png(width, height, pixels):
    header = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    data = SIGNATURE + chunk(b'IHDR', header)
    data += chunk(b'IDAT', compressed(pixels, width, height)) + chunk(b'IEND', b'')
    if decode(data)[2] != pixels:
        raise ValueError('PNG pixel round-trip mismatch')
    return data


def apng(size, frames, delays):
    header = struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0)
    data = SIGNATURE + chunk(b'IHDR', header) + chunk(b'acTL', struct.pack('>II', 2, 0))
    seq = 0
    for index, (frame, delay) in enumerate(zip(frames, delays)):
        data += chunk(b'fcTL', struct.pack('>IIIIIHHBB', seq, size, size, 0, 0,
                                         delay, 1000, 0, 0))
        seq += 1
        packed = compressed(frame, size, size)
        if index == 0:
            data += chunk(b'IDAT', packed)
        else:
            data += chunk(b'fdAT', struct.pack('>I', seq) + packed)
            seq += 1
    data += chunk(b'IEND', b'')
    parts = list(chunks(data))
    if len([v for k, v in parts if k == b'fcTL']) != 2:
        raise ValueError('APNG frame count mismatch')
    packed = [v for k, v in parts if k == b'IDAT'] + [v[4:] for k, v in parts if k == b'fdAT']
    for index, payload in enumerate(packed):
        frame_png = SIGNATURE + chunk(b'IHDR', header) + chunk(b'IDAT', payload) + chunk(b'IEND', b'')
        if decode(frame_png)[2] != frames[index]:
            raise ValueError('APNG pixel round-trip mismatch')
    return data


def main():
    width, height, pixels = decode(SOURCE.read_bytes())
    boxes = silhouettes(width, height, pixels)
    frames, metadata = [], []
    for index, (x0, y0, x1, y1) in enumerate(boxes):
        # Keep a small ring of the original antialiased edge pixels as well.
        sx, sy = max(0, x0 - 4), max(0, y0 - 4)
        ex, ey = min(width, x1 + 5), min(height, y1 + 5)
        fw, fh = ex - sx, ey - sy
        for j, other in enumerate(boxes):
            if j != index and sx <= other[2] and ex > other[0] and sy <= other[3] and ey > other[1]:
                raise ValueError('A crop would include a neighboring silhouette')
        frame = bytearray()
        for y in range(sy, ey):
            start = (y * width + sx) * 4
            frame.extend(pixels[start:start + fw * 4])
        solid = [(x, y) for y in range(fh) for x in range(fw)
                 if frame[(y * fw + x) * 4 + 3] >= 160]
        head = [(x, y) for x, y in solid if fh * .20 <= y <= fh * .45]
        feet = [(x, y) for x, y in solid if fw * .27 <= x <= fw * .73 and y > fh * .72]
        if not head or not feet:
            raise ValueError('Cannot locate alignment anchor')
        anchor = [round((min(x for x, y in head) + max(x for x, y in head)) / 2),
                  max(y for x, y in feet)]
        group, delays = GROUPS[index // 2]
        metadata.append({'index': index, 'state': group,
                         'eyes': 'open' if index % 2 == 0 else 'closed',
                         'sourceRect': [sx, sy, fw, fh], 'sourceAnchor': anchor,
                         'durationMs': delays[index % 2]})
        frames.append(frame)

    left = max(m['sourceAnchor'][0] for m in metadata)
    right = max(m['sourceRect'][2] - m['sourceAnchor'][0] for m in metadata)
    above = max(m['sourceAnchor'][1] for m in metadata)
    below = max(m['sourceRect'][3] - m['sourceAnchor'][1] for m in metadata)
    content_w, content_h = left + right + 24, above + below + 24
    size = max(content_w, content_h)
    anchor = [left + 12 + (size - content_w) // 2,
              above + 12 + (size - content_h) // 2]
    aligned = []
    for frame, meta in zip(frames, metadata):
        fw, fh = meta['sourceRect'][2:]
        dx, dy = [a - b for a, b in zip(anchor, meta['sourceAnchor'])]
        output = bytearray(size * size * 4)
        for y in range(fh):
            start = ((dy + y) * size + dx) * 4
            output[start:start + fw * 4] = frame[y * fw * 4:(y + 1) * fw * 4]
        meta['offset'] = [dx, dy]
        meta['sheetRect'] = [(meta['index'] % 2) * size, (meta['index'] // 2) * size, size, size]
        aligned.append(output)

    sheet_w, sheet_h = size * 2, size * 3
    sheet = bytearray(sheet_w * sheet_h * 4)
    for index, frame in enumerate(aligned):
        sx, sy = index % 2 * size, index // 2 * size
        for y in range(size):
            start = ((sy + y) * sheet_w + sx) * 4
            sheet[start:start + size * 4] = frame[y * size * 4:(y + 1) * size * 4]
    (ASSETS / 'emotions-sheet-v1.png').write_bytes(png(sheet_w, sheet_h, sheet))
    previews = []
    for row, (name, delays) in enumerate(GROUPS):
        path = ASSETS / f'{name}-preview-v1.apng'
        path.write_bytes(apng(size, aligned[row * 2:row * 2 + 2], delays))
        still_path = ASSETS / f'{name}-still-v1.png'
        still_path.write_bytes(png(size, size, aligned[row * 2]))
        previews.append({'state': name, 'file': path.name, 'stillFile': still_path.name,
                         'durationsMs': delays})
    manifest = {'source': SOURCE.name, 'sourceSha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                'sourceSize': [width, height], 'columns': 2, 'rows': 3,
                'sheetSize': [sheet_w, sheet_h], 'cellSize': [size, size],
                'anchor': anchor, 'frames': metadata, 'previews': previews,
                'status': 'user-approved-emotion-previews-web-integration-pending-visual-review-2026-10-21'}
    (ASSETS / 'emotions-sheet-v1.json').write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'sourceSize': [width, height], 'sheetSize': [sheet_w, sheet_h],
                      'cellSize': [size, size], 'sourceBounds': boxes,
                      'verified': '6 separate silhouettes; PNG CRC and 6 lossless APNG frame round trips; normalized PNG round trip',
                      'previews': previews}, ensure_ascii=False))


if __name__ == '__main__':
    main()
