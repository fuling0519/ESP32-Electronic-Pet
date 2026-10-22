"""Package exported shared-renderer scene frames into stable-palette GIF previews."""
from pathlib import Path
from PIL import Image
import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('frames_dir', type=Path)
    args = parser.parse_args()
    dest = Path(__file__).resolve().parents[1] / 'web-preview/assets/shared/cleaning/animation-v1'
    for pet in ('bird-baby', 'bird-adult', 'wyvern-baby', 'wyvern-adult'):
        paths = sorted((args.frames_dir / pet).glob('*.png'))
        assert len(paths) == 54, (pet, len(paths))
        rgb = [Image.open(p).convert('RGB') for p in paths]
        montage = Image.new('RGB', (384 * 3, 256 * 2))
        for i, index in enumerate((0, 10, 20, 28, 35, 48)):
            montage.paste(rgb[index], ((i % 3) * 384, (i // 3) * 256))
        palette = montage.quantize(colors=256)
        frames = [im.quantize(palette=palette, dither=Image.Dither.NONE) for im in rgb]
        out = dest / (pet + '-cleaning-preview.gif')
        frames[0].save(out, save_all=True, append_images=frames[1:], duration=[80] * 53 + [1000], loop=0, disposal=2, optimize=False)
        with Image.open(out) as check:
            assert check.size == (384, 256), pet
            total = 0
            for i in range(check.n_frames):
                check.seek(i)
                total += check.info['duration']
            assert total == 5240, (pet, total)
        print(f'{pet}: GIF {check.n_frames} frames, 384x256, 5240 ms including rest.')


if __name__ == '__main__':
    main()
