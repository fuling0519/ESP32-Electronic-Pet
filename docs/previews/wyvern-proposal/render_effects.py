"""Proposal: original dragon sprites with current firmware sparkle/jump rules."""
import runpy
from pathlib import Path
from PIL import Image, ImageDraw

helper=runpy.run_path(str(Path(__file__).with_name('render.py')))
OUT=helper['OUT'];home=helper['home'];old=helper['old'];place=helper['place']
sprite=helper['sprite'];sheet=helper['sheet'];treatment=helper['treatment'];FONT=helper['FONT']

def sparkles(im,frame,left,right):
    left_light=[0,1,2,3,2,1,0,1,2,3,2,1,0,1,2,0]
    right_light=[0,0,1,2,3,2,1,0,0,1,2,3,2,1,0,0]
    step=frame%16;d=ImageDraw.Draw(im)
    def star(x,y,light):
        if not light:return
        d.point((x,y),fill=255)
        if light>=2:
            d.line((x-1,y,x+1,y),fill=255);d.line((x,y-1,x,y+1),fill=255)
        if light>=3:
            d.line((x-2,y,x+2,y),fill=255);d.line((x,y-2,x,y+2),fill=255)
    star(left,39 if 7<=step<=11 else 23,left_light[step])
    star(right,25 if step>=9 else 37,right_light[step])
    return im

def recovery(stage,frame):
    # Firmware holds restored normal sprite frame zero during recovery.
    return sparkles(home(stage,'idle',0),frame,33,94)

def level(stage,frame):
    phase=frame%8;jump=2 if phase in (2,3) else 1 if phase in (1,4) else 0
    im=old('baby-level-up-0' if stage=='baby' else 'level-up-0')
    ImageDraw.Draw(im).rectangle((0,11,127,52),fill=0)
    # Two pixels down gives a full black row between horns and title at apex.
    base=9 if stage=='adult' else 7
    place(im,sprite(stage,'idle',phase//4),(32,base-jump))
    return sparkles(im,frame,18,109)

def paired(baby,adult,title):
    im=Image.new('RGB',(1080,320),'#181d24');d=ImageDraw.Draw(im)
    d.text((12,8),title+'｜幼龍',font=FONT,fill='white')
    d.text((552,8),title+'｜成龍',font=FONT,fill='white')
    im.paste(baby.convert('RGB').resize((512,256),Image.Resampling.NEAREST),(12,44))
    im.paste(adult.convert('RGB').resize((512,256),Image.Resampling.NEAREST),(552,44))
    return im

def save_pair(name,title,callback,count,durations):
    frames=[paired(callback('baby',i),callback('adult',i),title) for i in range(count)]
    frames[0].save(OUT/(name+'.gif'),save_all=True,append_images=frames[1:],duration=durations,loop=0)
    for stage in ('baby','adult'):
        native=[callback(stage,i) for i in range(count)]
        for i,im in enumerate(native):im.save(OUT/f'{name}-{stage}-{i:02d}-128x64.png')

# Firmware frames are 125ms; GIF rounds to 10ms. Alternating 120/130 totals 2s.
effect_durations=[120,130]*8
save_pair('recovery-v4','恢復閃光・共 2 秒',recovery,16,effect_durations)
save_pair('level-up-v4','升等・共 2 秒',level,16,effect_durations)
sheet('effects-keyframes-v4','關鍵幀提案｜恢復閃光與升等｜角色原始像素、既有特效節奏',[
    (prefix+'・恢復閃光',recovery(stage,3)) for stage,prefix in [('baby','幼龍'),('adult','成龍')]
]+[(prefix+'・升等最高點',level(stage,2)) for stage,prefix in [('baby','幼龍'),('adult','成龍')]])

frames=[paired(treatment('baby',f),treatment('adult',f),'倒藥・隱藏三條線') for f in range(5)]
frames += [paired(recovery('baby',f),recovery('adult',f),'治療成功・恢復閃光') for f in range(16)]
frames[0].save(OUT/'treatment-full-v4.gif',save_all=True,append_images=frames[1:],
               duration=[250,250,350,350,200]+effect_durations,loop=0)
print('Saved recovery, level-up, and full treatment proposal previews.')
