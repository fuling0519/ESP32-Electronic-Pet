"""Package the generated 2x2 RGBA sheet as an aligned, lossless APNG preview.

This only slices and places existing pixels; it does not repaint the artwork.
The original sheet is retained. Requires only the Python standard library.
"""
import json
import hashlib
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'web-preview/assets/pets/wyvern/adult'
SOURCE = ASSETS / 'idle-subtle-source-v2.png'
DELAYS = [220, 160, 220, 320]  # Web preview timing, in milliseconds; not firmware timing.
SIGNATURE = b'\x89PNG\r\n\x1a\n'


def chunks(data):
    if data[:8] != SIGNATURE:
        raise ValueError('Expected a PNG')
    pos = 8
    while pos < len(data):
        size = struct.unpack_from('>I', data, pos)[0]
        kind, payload = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + size]
        expected = struct.unpack_from('>I', data, pos + 8 + size)[0]
        if zlib.crc32(kind + payload) & 0xffffffff != expected:
            raise ValueError('PNG chunk CRC mismatch')
        yield kind, payload
        pos += size + 12


def decode(data):
    parts = list(chunks(data))
    width, height, depth, color, compression, filtering, interlace = struct.unpack(
        '>IIBBBBB', next(v for k, v in parts if k == b'IHDR'))
    if (depth, color, compression, filtering, interlace) != (8, 6, 0, 0, 0):
        raise ValueError('Expected non-interlaced 8-bit RGBA')
    raw = zlib.decompress(b''.join(v for k, v in parts if k == b'IDAT'))
    stride = width * 4
    if len(raw) != height * (stride + 1):
        raise ValueError('Unexpected scanline length')
    pixels, previous = bytearray(), bytearray(stride)
    for row in range(height):
        offset = row * (stride + 1)
        method = raw[offset]
        line = bytearray(raw[offset + 1:offset + stride + 1])
        for i in range(stride):
            left = line[i - 4] if i >= 4 else 0
            above, upper_left = previous[i], previous[i - 4] if i >= 4 else 0
            if method == 1:
                predictor = left
            elif method == 2:
                predictor = above
            elif method == 3:
                predictor = (left + above) // 2
            elif method == 4:
                p = left + above - upper_left
                distances = [abs(p - left), abs(p - above), abs(p - upper_left)]
                predictor = [left, above, upper_left][distances.index(min(distances))]
            elif method == 0:
                predictor = 0
            else:
                raise ValueError('Unknown PNG filter')
            line[i] = (line[i] + predictor) & 255
        pixels.extend(line)
        previous = line
    return width, height, pixels


def chunk(kind, payload):
    return struct.pack('>I', len(payload)) + kind + payload + struct.pack(
        '>I', zlib.crc32(kind + payload) & 0xffffffff)


def compressed(pixels, width, height):
    stride, rows = width * 4, bytearray()
    for y in range(height):
        line = pixels[y * stride:(y + 1) * stride]
        rows.append(1)  # PNG Sub filter; lossless.
        rows.extend((v - (line[i - 4] if i >= 4 else 0)) & 255
                    for i, v in enumerate(line))
    return zlib.compress(rows, 9)


def main():
    width, height, pixels = decode(SOURCE.read_bytes())
    if width % 2 or height % 2:
        raise ValueError('Sheet must divide into equal 2x2 cells')
    cell_w, cell_h = width // 2, height // 2
    frames, metadata = [], []
    for index in range(4):
        sx, sy = (index % 2) * cell_w, (index // 2) * cell_h
        frame = bytearray()
        for y in range(cell_h):
            start = ((sy + y) * width + sx) * 4
            frame.extend(pixels[start:start + cell_w * 4])
        solid = [(x, y) for y in range(cell_h) for x in range(cell_w)
                 if frame[(y * cell_w + x) * 4 + 3] >= 160]
        if not solid:
            raise ValueError('Empty frame')
        head = [(x, y) for x, y in solid if cell_h * .25 <= y <= cell_h * .44]
        feet = [(x, y) for x, y in solid if cell_w * .32 <= x <= cell_w * .68
                and y > cell_h * .72]
        if not head or not feet:
            raise ValueError('Cannot locate head or foot anchor')
        anchor_x = round((min(x for x, y in head) + max(x for x, y in head)) / 2)
        anchor_y = max(y for x, y in feet)
        bounds = [min(x for x, y in solid), min(y for x, y in solid),
                  max(x for x, y in solid), max(y for x, y in solid)]
        metadata.append({'index': index, 'sourceRect': [sx, sy, cell_w, cell_h],
                         'sourceAnchor': [anchor_x, anchor_y], 'bounds': bounds,
                         'durationMs': DELAYS[index]})
        frames.append(frame)

    # Include the entire source cells, including semi-transparent edge pixels.
    left = max(m['sourceAnchor'][0] for m in metadata)
    right = max(cell_w - m['sourceAnchor'][0] for m in metadata)
    above = max(m['sourceAnchor'][1] for m in metadata)
    below = max(cell_h - m['sourceAnchor'][1] for m in metadata)
    content_w, content_h = left + right + 16, above + below + 16
    out_w = out_h = max(content_w, content_h)
    target_anchor = [left + 8 + (out_w - content_w) // 2,
                     above + 8 + (out_h - content_h) // 2]
    aligned = []
    for frame, meta in zip(frames, metadata):
        dx, dy = [a - b for a, b in zip(target_anchor, meta['sourceAnchor'])]
        output = bytearray(out_w * out_h * 4)
        for y in range(cell_h):
            target = ((y + dy) * out_w + dx) * 4
            output[target:target + cell_w * 4] = frame[y * cell_w * 4:(y + 1) * cell_w * 4]
        meta['previewOffset'] = [dx, dy]
        meta['sheetRect'] = [(meta['index'] % 2) * out_w,
                             (meta['index'] // 2) * out_h, out_w, out_h]
        aligned.append(output)

    header = struct.pack('>IIBBBBB', out_w, out_h, 8, 6, 0, 0, 0)
    apng = SIGNATURE + chunk(b'IHDR', header) + chunk(b'acTL', struct.pack('>II', 4, 0))
    sequence = 0
    for index, frame in enumerate(aligned):
        control = struct.pack('>IIIIIHHBB', sequence, out_w, out_h, 0, 0,
                              DELAYS[index], 1000, 0, 0)
        apng += chunk(b'fcTL', control)
        sequence += 1
        data = compressed(frame, out_w, out_h)
        if index == 0:
            apng += chunk(b'IDAT', data)
        else:
            apng += chunk(b'fdAT', struct.pack('>I', sequence) + data)
            sequence += 1
    apng += chunk(b'IEND', b'')
    # Verify the generated file's CRCs and every frame's exact pixel round trip.
    parts = list(chunks(apng))
    if sum(k == b'fcTL' for k, v in parts) != 4:
        raise ValueError('APNG frame count mismatch')
    packed_frames = [v for k, v in parts if k == b'IDAT'] + [
        v[4:] for k, v in parts if k == b'fdAT']
    for index, packed in enumerate(packed_frames):
        standalone = SIGNATURE + chunk(b'IHDR', header) + chunk(b'IDAT', packed) + chunk(b'IEND', b'')
        if decode(standalone)[2] != aligned[index]:
            raise ValueError('Preview frame round-trip mismatch')
    output_path = ASSETS / 'idle-subtle-preview-v2.apng'
    output_path.write_bytes(apng)
    sheet_w, sheet_h = out_w * 2, out_h * 2
    sheet_pixels = bytearray(sheet_w * sheet_h * 4)
    for index, frame in enumerate(aligned):
        sx, sy = (index % 2) * out_w, (index // 2) * out_h
        for y in range(out_h):
            offset = ((sy + y) * sheet_w + sx) * 4
            sheet_pixels[offset:offset + out_w * 4] = frame[y * out_w * 4:(y + 1) * out_w * 4]
    sheet = SIGNATURE + chunk(b'IHDR', struct.pack('>IIBBBBB', sheet_w, sheet_h, 8, 6, 0, 0, 0))
    sheet += chunk(b'IDAT', compressed(sheet_pixels, sheet_w, sheet_h)) + chunk(b'IEND', b'')
    if decode(sheet)[2] != sheet_pixels:
        raise ValueError('Normalized sheet round-trip mismatch')
    (ASSETS / 'idle-subtle-sheet-v2.png').write_bytes(sheet)
    manifest = {'source': SOURCE.name, 'columns': 2, 'rows': 2,
                'sourceSha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                'sourceSize': [width, height], 'previewSize': [out_w, out_h],
                'sheetSize': [sheet_w, sheet_h], 'cellSize': [out_w, out_h],
                'previewAnchor': target_anchor, 'cycleMs': sum(DELAYS),
                'status': 'user-approved-idle-preview-2026-10-20',
                'frames': metadata}
    (ASSETS / 'idle-subtle-sheet-v2.json').write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'sourceSize': [width, height], 'cellSize': [cell_w, cell_h],
                      'previewSize': [out_w, out_h], 'cycleMs': sum(DELAYS),
                      'anchors': [m['sourceAnchor'] for m in metadata],
                      'verified': 'PNG CRC, 4 lossless APNG frame round trips, lossless normalized sheet round trip',
                      'preview': str(output_path)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
