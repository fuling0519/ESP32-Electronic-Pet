"""Render the actual farewell UI framebuffers produced by test_rps.ps1."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps, ImageChops
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return ImageOps.invert(Image.open(ROOT / '.pio/rps-preview' / f'{name}.pbm').convert('RGB'))


def main():
    composite = Image.new('RGB', (1048, 600), '#1c1e22')
    composite_draw = ImageDraw.Draw(composite)
    for row, stage in enumerate(('adult', 'baby')):
        for col, kind in enumerate(('departed', 'resting')):
            name = 'memorial-composite-' + kind + '-' + stage
            actual = read(name)
            expected = Image.open(ROOT / 'docs' / (name + '-proposal.png')).convert('RGB')
            assert ImageChops.difference(actual, expected).getbbox() is None, name + ' differs from approved preview'
            x, y = 6 + col * 524, 6 + row * 300
            composite_draw.text((x, y), kind + ' / ' + stage + ' / actual OLED framebuffer', fill='white')
            composite.paste(actual.resize((512,256), Image.Resampling.NEAREST), (x,y+18))
    composite.save(ROOT / 'docs/memorial-composite-implemented.png')
    print('PASS: all four memorial framebuffers match approved 128x64 previews pixel for pixel.')
    if '--composite-only' in sys.argv:
        return
    navigation = Image.new('RGB', (1048, 600), '#1c1e22')
    navigation_draw = ImageDraw.Draw(navigation)
    for i, name in enumerate(('album-categories-mixed', 'album-help', 'album-resting-sample', 'album-delete-confirm')):
        x, y = (i % 2) * 524 + 6, (i // 2) * 300 + 6
        navigation_draw.text((x, y), name, fill='white')
        navigation.paste(read(name).resize((512, 256), Image.Resampling.NEAREST), (x, y + 18))
    navigation.save(ROOT / 'docs/memorial-navigation-preview.png')
    records = Image.new('RGB', (1048, 600), '#1c1e22')
    records_draw = ImageDraw.Draw(records)
    for i, name in enumerate(('album-adult', 'album-baby', 'album-resting-sample', 'album-resting-empty')):
        x, y = (i % 2) * 524 + 6, (i // 2) * 300 + 6
        records_draw.text((x, y), name, fill='white')
        records.paste(read(name).resize((512, 256), Image.Resampling.NEAREST), (x, y + 18))
    records.save(ROOT / 'docs/memorial-record-layout.png')
    if '--memorial-only' in sys.argv:
        return
    names = [
        'farewell-info', 'farewell-confirm-keep', 'farewell-confirm-send',
        'farewell-adult-start', 'farewell-adult-blink', 'farewell-flash',
        'farewell-done', 'album-adult', 'album-baby',
        'album-resting', 'album-resting-empty', 'farewell-full',
    ]
    sheet = Image.new('RGB', (1212, 960), '#1c1e22')
    draw = ImageDraw.Draw(sheet)
    for i, name in enumerate(names):
        x, y = (i % 3) * 404 + 10, (i // 3) * 240 + 10
        sheet.paste(read(name).resize((384, 192), Image.Resampling.NEAREST), (x, y))
        draw.text((x, y + 202), name, fill='white')
    sheet.save(ROOT / 'docs/farewell-preview.png')
    album = Image.new('RGB', (1048, 608), '#1c1e22')
    albumDraw = ImageDraw.Draw(album)
    for i, name in enumerate(('album-adult', 'album-baby', 'album-adult-filter', 'album-baby-filter')):
        x, y = (i % 2) * 524 + 6, (i // 2) * 304 + 6
        albumDraw.text((x, y), name, fill='white')
        album.paste(read(name).resize((512, 256), Image.Resampling.NEAREST), (x, y + 22))
    album.save(ROOT / 'docs/album-layout-fix.png')
    for stage in ('adult', 'baby'):
        frames = [read(f'farewell-{stage}-start')]
        frames += [read(f'farewell-{stage}-{i:02}') for i in range(1, 28)]
        frames = [im.resize((512, 256), Image.Resampling.NEAREST) for im in frames]
        frames[0].save(ROOT / f'docs/farewell-{stage}-preview.gif', save_all=True,
                       append_images=frames[1:], duration=[100] * 27 + [2400], loop=0)


if __name__ == '__main__':
    main()
