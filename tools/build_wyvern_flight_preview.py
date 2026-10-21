"""Package approved-to-generate flight art for review; no repainting or rescaling.

Requires Pillow (available in the Codex bundled Python runtime).
APNG and frame PNGs preserve the original RGBA pixels exactly. The GIF is a
palette-limited viewing copy on cream; it is not the transparent source asset.
"""
import hashlib
import json
import math
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'web-preview/assets/pets/wyvern/adult'
SOURCE = ASSETS / 'flight-source-v1.png'
# Exclusive silhouette bounds checked against the actual source Alpha channel.
BOUNDS = [(32, 82, 495, 475), (522, 128, 1015, 475),
          (1032, 135, 1531, 482), (22, 617, 519, 946),
          (535, 607, 1036, 953), (1062, 566, 1522, 953)]
# Center of the large, near-side eye; integer translation only.
ANCHORS = [(360, 294), (846, 291), (1363, 294),
           (342, 772), (848, 771), (1385, 772)]
DELAYS = [170, 130, 130, 170, 130, 170]


def main():
    source = Image.open(SOURCE).convert('RGBA')
    assert source.size == (1536, 1024)
    rects = [(max(0,x0-8), max(0,y0-8), min(source.width,x1+8), min(source.height,y1+8))
             for x0, y0, x1, y1 in BOUNDS]
    left = max(ax-r[0] for (ax, ay), r in zip(ANCHORS, rects))
    right = max(r[2]-ax for (ax, ay), r in zip(ANCHORS, rects))
    above = max(ay-r[1] for (ax, ay), r in zip(ANCHORS, rects))
    below = max(r[3]-ay for (ax, ay), r in zip(ANCHORS, rects))
    size = (math.ceil((left+right+24)/16)*16,
            math.ceil((above+below+24)/16)*16)
    target = (left+12, above+12)
    frames, metadata = [], []
    for i, (rect, anchor, delay) in enumerate(zip(rects, ANCHORS, DELAYS)):
        assert 0 <= rect[0] < rect[2] <= source.width
        assert 0 <= rect[1] < rect[3] <= source.height
        crop = source.crop(rect)
        offset = (target[0]-(anchor[0]-rect[0]), target[1]-(anchor[1]-rect[1]))
        frame = Image.new('RGBA', size)
        frame.paste(crop, offset)  # No alpha mask: exact RGBA copy, not compositing.
        assert frame.crop((*offset, offset[0]+crop.width, offset[1]+crop.height)).tobytes() == crop.tobytes()
        frame.save(ASSETS / f'flight-frame-{i:02d}-v1.png')
        frames.append(frame)
        metadata.append({'index': i, 'sourceRect': [rect[0], rect[1], crop.width, crop.height],
                         'sourceAnchor': list(anchor), 'previewOffset': list(offset),
                         'durationMs': delay})

    animation = ASSETS / 'flight-preview-v1.apng'
    frames[0].save(animation, format='PNG', save_all=True, append_images=frames[1:],
                   duration=DELAYS, loop=0, disposal=0, blend=0, optimize=False)
    with Image.open(animation) as check:
        assert check.n_frames == 6 and check.info['loop'] == 0
        for i, expected in enumerate(frames):
            check.seek(i)
            assert check.convert('RGBA').tobytes() == expected.tobytes()
            assert round(check.info['duration']) == DELAYS[i]

    sheet = Image.new('RGBA', (size[0]*3, size[1]*2))
    for i, frame in enumerate(frames):
        sheet.paste(frame, ((i%3)*size[0], (i//3)*size[1]))
    sheet.save(ASSETS / 'flight-sheet-v1.png')

    cream = (248, 245, 236, 255)
    rgb = [Image.alpha_composite(Image.new('RGBA', size, cream), f).convert('RGB') for f in frames]
    palette_source = Image.new('RGB', sheet.size, cream[:3])
    for i, frame in enumerate(rgb):
        palette_source.paste(frame, ((i%3)*size[0], (i//3)*size[1]))
    palette = palette_source.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    gif_frames = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in rgb]
    gif = ASSETS / 'flight-preview-v1.gif'
    gif_frames[0].save(gif, save_all=True, append_images=gif_frames[1:],
                       duration=DELAYS, loop=0, disposal=2, optimize=False)
    with Image.open(gif) as check:
        assert check.n_frames == 6 and check.info['loop'] == 0
        for i, expected in enumerate(gif_frames):
            check.seek(i)
            assert check.convert('RGB').tobytes() == expected.convert('RGB').tobytes()
            assert check.info['duration'] == DELAYS[i]
    manifest = {'source': SOURCE.name, 'sourceSha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                'sourceSize': list(source.size), 'cellSize': list(size), 'columns': 3, 'rows': 2,
                'previewAnchor': list(target), 'cycleMs': sum(DELAYS), 'loop': 0,
                'status': 'flight-cycle-authorized-for-web-integration-20261021',
                'renderAnchor': [280, 310], 'renderScale': 172/517,
                'operations': ['crop', 'integer-translation', 'transparent-padding'],
                'gifNote': 'Cream viewing copy, shared 256-color palette; APNG retains exact RGBA.',
                'verified': ['6 exact RGBA APNG frame round trips', '6 exact GIF palette frame round trips', 'frame timing', 'infinite loop'],
                'frames': metadata}
    (ASSETS / 'flight-sheet-v1.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'previewSize': size, 'cycleMs': sum(DELAYS), 'verified': manifest['verified'], 'gif': str(gif)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
