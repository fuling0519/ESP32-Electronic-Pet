"""Validate the six-frame study, preserving the generated RGBA cells verbatim."""
from pathlib import Path
import hashlib
import json
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'web-preview/assets/shared/cleaning/foam-study-v2'


def main():
    source = DEST / 'foam-source-v2.png'
    im = Image.open(source)
    assert im.mode == 'RGBA' and im.size == (1536, 1024)
    frames = []
    for i in range(6):
        x, y = i % 3 * 512, i // 3 * 512
        cell = im.crop((x, y, x + 512, y + 512))
        alpha = cell.getchannel('A')
        bounds = alpha.point(lambda a: 255 if a >= 128 else 0).getbbox()
        assert bounds and min(bounds[:2]) > 0 and max(bounds[2:]) < 512
        assert alpha.getextrema()[0] == 0
        frames.append({'index': i, 'visibleBounds': list(bounds), 'alphaArea': sum(alpha.tobytes()) / 255})
    assert frames[0]['alphaArea'] < frames[1]['alphaArea'] < frames[2]['alphaArea']
    assert frames[2]['alphaArea'] > frames[3]['alphaArea'] > frames[4]['alphaArea'] > frames[5]['alphaArea']
    atlas = DEST / 'foam-sheet-v2.png'
    im.save(atlas)
    assert Image.open(atlas).tobytes() == im.tobytes()
    background = Image.new('RGBA', im.size, '#f7f5ed')
    background.alpha_composite(im)
    background.convert('RGB').save(DEST / 'source-on-cream.png')
    data = {'file': '../foam-study-v2/foam-sheet-v2.png', 'columns': 3, 'cellWidth': 512,
            'cellHeight': 512, 'frameCount': 6, 'anchor': [256, 280],
            'referenceWidth': frames[2]['visibleBounds'][2] - frames[2]['visibleBounds'][0],
            'suggestedWidth': 32}
    manifest = {'sampleOnly': True, 'shapeCount': 1, 'assets': {'foam': data}}
    (DEST / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    (DEST / 'foam-data.js').write_text('Object.assign(window.CleaningAnimationData.assets,' + json.dumps(manifest['assets'], separators=(',', ':')) + ');\n', encoding='utf-8')
    report = {'kind': 'file and alpha checks, not art acceptance', 'size': list(im.size),
              'alphaExtrema': list(im.getchannel('A').getextrema()), 'frames': frames,
              'sourceSha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'atlasSha256': hashlib.sha256(atlas.read_bytes()).hexdigest(),
              'sourceRgbaPreserved': True, 'transform': 'none'}
    (DEST / 'validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
