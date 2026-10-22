"""Merge and validate the six generated cleaning atlases without changing pixels."""
from pathlib import Path
from PIL import Image
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'web-preview/assets/shared/cleaning/animation-v1'
EXPECTED = {'sponge': 3, 'foam': 8, 'shower': 1, 'water': 6, 'drops': 5, 'sparkle': 5}


def main():
    assets = {}
    for group in ('group-a', 'group-b', 'group-c'):
        data = json.loads((DEST / group / 'manifest.json').read_text(encoding='utf-8-sig'))
        assert not (assets.keys() & data['assets'].keys()), 'Duplicate asset names'
        assets.update(data['assets'])
    assert set(assets) == set(EXPECTED), 'Missing or unexpected assets'
    report = {'kind': 'file/atlas validation, not visual acceptance', 'effectiveFrames': 0, 'assets': {}}
    for name, count in EXPECTED.items():
        data = assets[name]
        assert data['frameCount'] == count, (name, data['frameCount'])
        path = DEST / data['file']
        assert path.resolve().is_relative_to(DEST.resolve()), name
        im = Image.open(path)
        rows = (count + data['columns'] - 1) // data['columns']
        assert im.mode == 'RGBA', (name, im.mode)
        assert im.size == (data['cellWidth'] * data['columns'], data['cellHeight'] * rows), (name, im.size)
        assert im.getchannel('A').getextrema()[0] == 0, name
        assert data['referenceWidth'] > 0 and len(data['anchor']) == 2, name
        frames = []
        for i in range(rows * data['columns']):
            x, y = i % data['columns'] * data['cellWidth'], i // data['columns'] * data['cellHeight']
            cell = im.crop((x, y, x + data['cellWidth'], y + data['cellHeight']))
            alpha = cell.getchannel('A')
            bounds = alpha.point(lambda a: 255 if a >= 128 else 0).getbbox()
            if i < count:
                assert bounds is not None, (name, i, 'empty visible frame')
                frames.append({'index': i, 'visibleBounds': list(bounds)})
            else:
                assert alpha.getextrema() == (0, 0), (name, i, 'unused cell not transparent')
        report['assets'][name] = {'file': data['file'], 'size': list(im.size), 'alphaExtrema': list(im.getchannel('A').getextrema()), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'frames': frames}
        report['effectiveFrames'] += count
    assert report['effectiveFrames'] == 28
    ordered = {name: assets[name] for name in EXPECTED}
    # Measured main-flow endpoint from the generated water sheet, relative to its top anchor.
    ordered['water']['flowEnd'] = [-110, 350]
    merged = {'durationSeconds': 4.2, 'spongeReferenceDisplayWidth': 60, 'spongeShrinksForBaby': False, 'assets': ordered}
    (DEST / 'animation-data.json').write_text(json.dumps(merged, indent=2), encoding='utf-8')
    (DEST / 'animation-data.js').write_text('window.CleaningAnimationData=' + json.dumps(merged, separators=(',', ':')) + ';\n', encoding='utf-8')
    (DEST / 'atlas-validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'Validated {len(assets)} RGBA atlases and {report["effectiveFrames"]} effective frames; unused cells are transparent.')


if __name__ == '__main__':
    main()
