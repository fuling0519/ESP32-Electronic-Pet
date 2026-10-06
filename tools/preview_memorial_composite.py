"""Preview only: combine existing native OLED frames with a filled cross stone.

Does not change firmware or the user's original sprite pixels.
"""
from pathlib import Path
import re
from PIL import Image, ImageDraw, ImageFilter, ImageOps

ROOT = Path(__file__).resolve().parents[1]


def frame(name):
    return ImageOps.invert(Image.open(ROOT / '.pio/rps-preview' / (name + '.pbm')).convert('L'))


def sprite_portrait(stage, resting):
    # Decode the exact existing first sprite frame; no new eye artwork.
    header = (ROOT / 'src/ui/BirdSprite.h').read_text(encoding='utf-8')
    symbol = ('kBabyBird' if stage == 'baby' else 'kBird') + ('SleepingFrames' if resting else 'Frames')
    block = re.search(r'const uint8_t ' + symbol + r'\[2\]\[352\].*?=\s*\{(.*?)\};', header, re.S)
    values = [int(value,16) for value in re.findall(r'0x([0-9A-Fa-f]{2})', block.group(1))][:352]
    source = Image.new('L', (64,44))
    for y in range(44):
        for x in range(64):
            if not (values[y*8+x//8] & (128 >> (x%8))):
                source.putpixel((x,y),255)
    _, top, _, bottom = source.getbbox()
    portrait = Image.new('L', (64,42))
    portrait.paste(source.crop((0,top,64,bottom)), (0,(42-(bottom-top))//2))
    return portrait


def compose(stage, resting=True):
    result = frame('album-resting-sample' if resting else 'album-' + stage)
    # Leave header intact; use the same right text column as the farewell pet.
    text = result.crop((73, 25, 121, 53))
    ImageDraw.Draw(result).rectangle((1, 19, 126, 62), fill=0)
    result.paste(text, (73, 25), text)
    draw = ImageDraw.Draw(result)
    # Use the exact native portrait. One black pixel separates white objects.
    bird = sprite_portrait(stage, resting)
    assert bird.getbbox() == sprite_portrait(stage, False).getbbox()
    bird_x = -3  # Both categories move six pixels left from the native x=3.
    if resting:
        # Baby's visible height is 27px; this stone is only 13x14px.
        # Align the base with the same bird's feet, without resizing the bird.
        x, y = 53, 19 + bird.getbbox()[3] - 14
        shape = [(x+4,y), (x+8,y), (x+10,y+2), (x+10,y+11),
                 (x+12,y+13), (x,y+13), (x+2,y+11), (x+2,y+2)]
        draw.polygon(shape, fill=255)
        draw.line((x+6,y+3,x+6,y+9), fill=0)
        draw.line((x+4,y+5,x+8,y+5), fill=0)
    silhouette = Image.new('L', bird.size)
    mask_draw = ImageDraw.Draw(silhouette)
    for row in range(bird.height):
        visible = [col for col in range(bird.width) if bird.getpixel((col,row))]
        if visible:
            mask_draw.line((min(visible),row,max(visible),row), fill=255)
    expanded = Image.new('L', (128,64))
    expanded.paste(silhouette, (bird_x,19))
    result.paste(0, (0,0,128,64), expanded.filter(ImageFilter.MaxFilter(3)))
    result.paste(bird, (bird_x,19), bird)
    # Preview may cover background, but every original lit pet pixel survives.
    for row in range(bird.height):
        for col in range(bird.width):
            if bird.getpixel((col,row)):
                assert result.getpixel((bird_x+col,19+row)) == 255
    return result


def main():
    sheet = Image.new('RGB', (1048,600), '#1c1e22')
    labels = ImageDraw.Draw(sheet)
    for row, stage in enumerate(('adult','baby')):
        for col, resting in enumerate((False, True)):
            result = compose(stage, resting)
            kind = 'resting' if resting else 'departed'
            result.save(ROOT / 'docs' / ('memorial-composite-' + kind + '-' + stage + '-proposal.png'))
            sheet.paste(result.convert('RGB').resize((512,256), Image.Resampling.NEAREST), (6+col*524,24+row*300))
            labels.text((6+col*524,6+row*300), kind + ' / ' + stage + ' / 128x64 proposal', fill='white')
    sheet.save(ROOT / 'docs/memorial-composite-proposal.png')


if __name__ == '__main__':
    main()
