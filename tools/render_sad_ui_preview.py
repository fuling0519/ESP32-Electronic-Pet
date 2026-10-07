"""Verify approved bird/effect pixels in actual UI framebuffers and render."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps, ImageChops

ROOT = Path(__file__).resolve().parents[1]
sheet = Image.new('RGB', (1064, 600), '#202020')
draw = ImageDraw.Draw(sheet)
icon = Image.open(ROOT / 'assets/shared/effects/sad_lines.png').convert('RGBA')
for row, stage in enumerate(('baby', 'adult')):
    for frame in range(2):
        name = f'sad-{stage}-frame-{frame}'
        actual = ImageOps.invert(Image.open(ROOT / f'.pio/rps-preview/{name}.pbm').convert('L'))
        pet = Image.open(ROOT / f'assets/pets/bird/{stage}/sad.png').convert('RGBA')
        pet = pet.crop((0, frame * 44, 64, (frame + 1) * 44))
        pet.alpha_composite(icon, (40, 14) if stage == 'baby' else (39, 9))
        expected = Image.new('RGBA', (64, 44), 'black')
        expected.alpha_composite(pet)
        assert not ImageChops.difference(actual.crop((32, 6, 96, 50)), expected.convert('L')).getbbox(), name
        x, y = 12 + frame * 532, 12 + row * 300
        draw.text((x, y), name.upper(), fill='white')
        sheet.paste(actual.resize((512, 256), Image.Resampling.NEAREST), (x, y + 25))
sheet.save(ROOT / 'docs/sad-state-preview.png')
print('PASS: all four actual UI sprite regions match original birds + approved shared effect pixel-for-pixel.')
