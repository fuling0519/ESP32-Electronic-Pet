"""Compare actual sound-focus framebuffers to the approved six-state proposal."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps, ImageChops
ROOT = Path(__file__).resolve().parents[1]
sheet = Image.new('RGB', (1200,464), '#1c1e22')
draw = ImageDraw.Draw(sheet)
for row,mode in enumerate(('off','on')):
    for col,focus in enumerate(('edit','confirm','cancel')):
        name=mode+'-'+focus
        frame=ImageOps.invert(Image.open(ROOT / '.pio/rps-preview' / ('sound-focus-'+name+'.pbm')).convert('L'))
        assert frame.size==(128,64)
        proposal=Image.open(ROOT / 'docs' / ('sound-focus-'+name+'-proposal.png')).convert('L')
        assert not ImageChops.difference(frame,proposal).getbbox(),name
        frame.save(ROOT / 'docs' / ('sound-focus-'+name+'-preview.png'))
        x,y=col*400+8,row*232
        draw.text((x,y+8),name.upper()+' / ACTUAL UI / 128x64',fill='white')
        sheet.paste(frame.resize((384,192),Image.Resampling.NEAREST),(x,y+28))
for name in ('sound-menu','sound-focus-save-failed'):
    frame=ImageOps.invert(Image.open(ROOT / '.pio/rps-preview' / (name+'.pbm')).convert('L'))
    frame.save(ROOT / 'docs' / (name+'-preview.png'))
sheet.save(ROOT / 'docs/sound-focus-ui-preview.png')
print('PASS: all six sound focus framebuffers match approved proposal pixel-for-pixel.')
