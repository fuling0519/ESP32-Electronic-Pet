"""Render the actual farewell UI framebuffers produced by test_rps.ps1."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return ImageOps.invert(Image.open(ROOT / '.pio/rps-preview' / f'{name}.pbm').convert('RGB'))


def main():
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
