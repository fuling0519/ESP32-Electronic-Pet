"""Render contact sheet and animation from real MemoryNotes UI framebuffers."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / '.pio/rps-preview'
NAMES = [
    'notes-select', 'notes-ready', 'notes-play-0', 'notes-gap',
    'notes-play-1', 'notes-play-2', 'notes-play-3', 'notes-answer',
    'notes-answer-flash', 'notes-correct', 'notes-wrong', 'notes-timeout',
    'notes-summary-0', 'notes-summary-3', 'notes-summary-4', 'notes-summary-5',
    'notes-level-up', 'notes-replay', 'notes-blocked-egg', 'notes-blocked-sick',
    'notes-play-six', 'notes-summary-1', 'notes-summary-2', 'notes-gap-last',
]

def load(name):
    # PBM ones mark lit OLED pixels, opposite to the PBM color convention.
    return Image.open(SOURCE / (name+'.pbm')).convert('L').point(lambda p: 255-p)

sheet = Image.new('RGB', (4*276,6*174),'#202020')
draw = ImageDraw.Draw(sheet)
for i,name in enumerate(NAMES):
    x,y=(i%4)*276+10,(i//4)*174+8
    draw.text((x,y),name,fill='white')
    sheet.paste(load(name).resize((256,128),Image.Resampling.NEAREST),(x,y+24))
sheet.save(ROOT/'docs/memory-notes-preview.png')
load('notes-summary-3').resize((768,384),Image.Resampling.NEAREST).save(
    ROOT/'docs/memory-notes-summary-preview.png')

frames=[load(name).resize((512,256),Image.Resampling.NEAREST)
        for name in ['notes-play-0','notes-gap','notes-play-last','notes-gap-last',
                     'notes-answer','notes-answer-flash','notes-answer',
                     'notes-final-answer-first','notes-correct-first']]
frames[0].save(ROOT/'docs/memory-notes-preview.gif',save_all=True,
               append_images=frames[1:],duration=[250,450,250,450,800,180,600,180,1000],loop=0)
