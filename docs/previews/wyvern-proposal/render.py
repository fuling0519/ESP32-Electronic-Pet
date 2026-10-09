"""Proposal only. Compose original sprite pixels over saved OLED preview HUDs."""
from pathlib import Path
import re
from PIL import Image, ImageDraw, ImageOps, ImageFont

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]
FONT = ImageFont.truetype('C:/Windows/Fonts/msjh.ttc', 19)
SMALL = ImageFont.load_default(size=8)

PIXELS = {
 'B':['1110','1001','1110','1001','1110'], 'A':['0110','1001','1111','1001','1001'],
 'Y':['1001','1001','0110','0010','0010'], 'L':['1000','1000','1000','1000','1111'],
 'v':['0000','0000','1001','1001','0110'], '1':['0100','1100','0100','0100','1110'],
 '0':['0110','1001','1001','1001','0110'], 'Z':['1111','0001','0010','0100','1111'],
 'z':['0000','0000','1110','0100','1110']}
def pixel_text(im,xy,text):
    for i,ch in enumerate(text):
        for y,row in enumerate(PIXELS[ch]):
            for x,value in enumerate(row):
                if value=='1': im.putpixel((xy[0]+i*5+x,xy[1]+y),255)

def chinese(im,xy,text):
    header=(ROOT/'include/ui/UiFont12.h').read_text(encoding='utf-8')
    for i,ch in enumerate(text):
        block=re.search(r'kGlyph'+f'{ord(ch):04X}'+r'\[24\].*?\{(.*?)\}',header,re.S)
        data=[int(x,16) for x in re.findall(r'0x([0-9A-F]{2})',block.group(1))]
        for y in range(12):
            for x in range(12):
                if not(data[y*2+x//8] & (128>>(x%8))):im.putpixel((xy[0]+i*12+x,xy[1]+y),255)

def sprite(stage, state, frame=0):
    im = Image.open(ROOT / f'assets/pets/wyvern/{stage}/{state}.png').convert('RGBA')
    cols, rows = im.width // 64, im.height // 44
    frame %= cols * rows
    x, y = frame % cols * 64, frame // cols * 44
    return im.crop((x, y, x+64, y+44)).getchannel('A')

def old(name):
    return ImageOps.invert(Image.open(ROOT / f'.pio/rps-preview/{name}.pbm').convert('L'))

def place(im, art, xy):
    im.paste(255, (xy[0],xy[1],xy[0]+art.width,xy[1]+art.height), art)

def home(stage, state='idle', frame=0, sad_lines=True):
    im = old('home-exp-half')
    d = ImageDraw.Draw(im)
    d.rectangle((20,0,108,54),fill=0)
    d.rectangle((103,55,127,63),fill=0)
    pixel_text(im,(108,58), 'BABY' if stage=='baby' else 'Lv10')
    place(im,sprite(stage,state,frame),(32,6))
    if state=='eating':
        # Both dragons' feet end at Home y=47. Bowl visible bottom is local y=12.
        bowl=Image.open(ROOT/'assets/shared/items/food_bowl.png').convert('RGBA')
        bowl=bowl.crop((0,0,21,15)).getchannel('A')
        place(im,bowl,(24,35))
    if state=='sad' and sad_lines:
        lines=Image.open(ROOT/'assets/shared/effects/sad_lines.png').convert('RGBA').getchannel('A')
        place(im,lines,(74,20) if stage=='baby' else (82,12))
    if state=='sleep':
        # Bird reference: small text begins near (78,12), baseline 17.
        pixel_text(im,(76,18) if stage=='baby' else (83,10),'Zzz')
    return im.point(lambda v:255 if v>=128 else 0)

def potion(frame):
    im=Image.open(ROOT/'assets/shared/items/potion.png').convert('RGBA').getchannel('A')
    x,y=((0,0),(21,0),(0,21),(21,21),(0,42))[frame]
    return im.crop((x,y,x+21,y+21))

def treatment(stage,frame,adjusted=True):
    im=home(stage,'sad',0,sad_lines=False)
    # Keep sad face, hide lines during pouring, and avoid adult horns.
    d=ImageDraw.Draw(im)
    for row in range(1,11):
        arm=4<=row<=7
        d.line((113+(1 if arm else 4),7+row,113+(10 if arm else 7),7+row),fill=255)
    place(im,potion(frame),(88,1) if adjusted and stage=='adult' else (78,1))
    return im

def card(stage, resting=False):
    im = old('album-'+stage)
    ImageDraw.Draw(im).rectangle((1,19,71,62),fill=0)
    ImageDraw.Draw(im).rectangle((73,45,121,62),fill=0)
    chinese(im,(85,48),'長眠' if resting else '遠行')
    art=sprite(stage,'sleep' if resting else 'idle')
    _,top,_,bottom=art.getbbox()
    y=19+(42-(bottom-top))//2-top
    place(im,art,(-3,y))
    if resting:
        x,sy=53,y+bottom-14
        d=ImageDraw.Draw(im)
        d.polygon([(x+4,sy),(x+8,sy),(x+10,sy+2),(x+10,sy+11),(x+12,sy+13),(x,sy+13),(x+2,sy+11),(x+2,sy+2)],fill=255)
        d.line((x+6,sy+3,x+6,sy+9),fill=0)
        d.line((x+4,sy+5,x+8,sy+5),fill=0)
    return im

def postcard(stage):
    im=old('farewell-'+stage+'-card')
    ImageDraw.Draw(im).rectangle((3,3,62,60),fill=0)
    art=sprite(stage,'idle');_,top,_,bottom=art.getbbox()
    place(im,art,(3,(64-(bottom-top))//2-top))
    return im

def sheet(name, title, entries, cols=2):
    rows=(len(entries)+cols-1)//cols
    im=Image.new('RGB',(cols*536+16,rows*306+65),'#181d24')
    d=ImageDraw.Draw(im);d.text((16,12),title,font=FONT,fill='#d8e8ef')
    for i,(label,frame) in enumerate(entries):
        x=16+i%cols*536;y=58+i//cols*306
        d.text((x,y),label,font=FONT,fill='white')
        im.paste(frame.convert('RGB').resize((512,256),Image.Resampling.NEAREST),(x,y+31))
        frame.save(OUT/f'{name}-{i+1}-128x64.png')
    im.save(OUT/f'{name}.png')

sheet('home','小飛龍提案｜原始像素 ×4｜上：幼龍／下：成龍',[
    (label,home(stage,'idle',frame)) for stage,label0 in [('baby','幼龍'),('adult','成龍')]
    for frame,label in [(0,label0+'・待機第 1 格'),(1,label0+'・待機第 2 格')]])
sheet('care','照護修訂｜食物碗、頭邊三條線、靠近頭部的 Zzz',[
    (label,home(stage,state)) for stage,prefix in [('baby','幼龍'),('adult','成龍')]
    for state,label in [('eating',prefix+'・餵食＋食物碗'),('sad',prefix+'・傷心＋三條線'),('sleep',prefix+'・睡眠＋Zzz')]],3)
sheet('treatment-compare','治療位置對照｜倒藥第 3 格｜原位置／成龍調整提案',[
    ('幼龍・原藥水位置',treatment('baby',2,False)),
    ('幼龍・保留原位置',treatment('baby',2)),
    ('成龍・原位置碰到 1 個龍像素',treatment('adult',2,False)),
    ('成龍・藥水右移，避開角與傷心線',treatment('adult',2))])
sheet('treatment-frames','治療倒藥逐格提案｜成龍藥水原點 (88,1)；幼龍 (78,1)',[
    (prefix+'・藥水第 '+str(f+1)+' 格',treatment(stage,f))
    for stage,prefix in [('baby','幼龍'),('adult','成龍')] for f in range(5)],5)
sheet('memorial','生命紀錄提案｜文字沿用既有測試畫面，僅示意排版',[
    (label,frame) for stage,prefix in [('baby','幼龍'),('adult','成龍')]
    for label,frame in [(prefix+'・送別明信片',postcard(stage)),(prefix+'・紀念冊遠行',card(stage)),(prefix+'・紀念冊長眠',card(stage,True))]],3)
for stage in ('baby','adult'):
    frames=[home(stage,'eating',i).resize((512,256),Image.Resampling.NEAREST) for i in range(4)]
    frames[0].save(OUT/f'eating-{stage}.gif',save_all=True,append_images=frames[1:],duration=375,loop=0)
    # GIF has 10 ms resolution: 160/170/170 ms averages exactly 6 FPS.
    frames=[home(stage,'idle',i%2).resize((512,256),Image.Resampling.NEAREST) for i in range(6)]
    frames[0].save(OUT/f'idle-{stage}.gif',save_all=True,append_images=frames[1:],duration=[160,170,170,160,170,170],loop=0)
    for state in ('sad','sleep'):
        frames=[home(stage,state,i).resize((512,256),Image.Resampling.NEAREST) for i in range(2)]
        frames[0].save(OUT/f'{state}-{stage}-v3.gif',save_all=True,append_images=frames[1:],duration=1000,loop=0)
    frames=[treatment(stage,f).resize((512,256),Image.Resampling.NEAREST) for f in range(5)]
    frames[0].save(OUT/f'treatment-{stage}-v3.gif',save_all=True,append_images=frames[1:],duration=[250,250,350,350,200],loop=0)
for name in ('care','treatment-compare','treatment-frames'):
    (OUT/f'{name}-v3.png').write_bytes((OUT/f'{name}.png').read_bytes())
print(OUT)
