"""Render acceptance previews from actual C++ UI/U8g2 framebuffers."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps, ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/previews/wyvern-implemented'
OUT.mkdir(parents=True,exist_ok=True)
FONT=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',19)
def read(stage,state):
    return ImageOps.invert(Image.open(ROOT/f'.pio/rps-preview/wyvern-{stage}-{state}.pbm').convert('L'))
def pair(state,label):
    image=Image.new('RGB',(1080,320),'#181d24');draw=ImageDraw.Draw(image)
    for i,(stage,name) in enumerate((('baby','幼龍'),('adult','成龍'))):
        draw.text((12+i*540,8),name+'・'+label,font=FONT,fill='white')
        frame=read(stage,state)
        frame.save(OUT/f'{stage}-{state}-128x64.png')
        image.paste(frame.convert('RGB').resize((512,256),Image.Resampling.NEAREST),(12+i*540,44))
    return image
def animation(name,states,durations):
    frames=[pair(state,name) for state in states]
    frames[0].save(OUT/(name+'.gif'),save_all=True,append_images=frames[1:],duration=durations,loop=0)
animation('idle-6fps',[f'idle-{i%2}' for i in range(6)],[160,170,170,160,170,170])
animation('sad-1fps',['sad-0','sad-1'],[1000,1000])
animation('sleep-1fps',['sleep-0','sleep-1'],[1000,1000])
animation('recovery',[f'recovery-{i}' for i in range(16)],[120,130]*8)
animation('level-up',[f'level-up-{i}' for i in range(16)],[120,130]*8)
animation('treatment',[f'pour-{i}' for i in range(5)]+[f'recovery-{i}' for i in range(16)],
          [250,250,350,350,200]+[120,130]*8)
rows=[('idle-0','待機'),('sad-0','傷心'),('sleep-1','睡眠'),('pour-2','治療・無三條線'),
      ('recovery-3','恢復閃光'),('level-up-2','升等')]
sheet=Image.new('RGB',(1080,320*len(rows)),'#181d24')
for i,(state,label) in enumerate(rows):sheet.paste(pair(state,label),(0,320*i))
sheet.save(OUT/'ui-preview.png')
print('Rendered actual wyvern firmware UI previews:',OUT)
