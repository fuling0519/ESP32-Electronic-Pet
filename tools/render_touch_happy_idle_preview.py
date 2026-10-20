"""V4 preview only: true idle poses + user happy face + independent hearts.

No canonical PNG, firmware header, UI or input source is written.
Requires Pillow, g++, and native U8g2 objects from tools/test_rps.ps1.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/previews/touch-proposal/user-art-v4'
WORK=ROOT/'.pio/touch-happy-idle-v4'
OUT.mkdir(parents=True,exist_ok=True);WORK.mkdir(parents=True,exist_ok=True)
ROLES=[
 ('bird-baby','幼鳥','bird','baby',[(13,14,18,19),(44,11,51,17)],'kBabyBirdFrames'),
 ('bird-adult','成鳥','bird','adult',[(6,12,11,17),(43,8,50,14)],'kBirdFrames'),
 ('wyvern-baby','幼龍','wyvern','baby',[(12,9,19,15),(48,14,53,19)],'kBabyWyvernIdleFrames'),
 ('wyvern-adult','成龍','wyvern','adult',[(5,19,12,25),(55,10,60,15)],'kWyvernIdleFrames'),
]
SOURCE_HASHES={}


def read_alpha(path):
    SOURCE_HASHES[path.relative_to(ROOT).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
    im=Image.open(path).convert('RGBA')
    assert all(im.getpixel((x,y)) in ((255,255,255,255),(0,0,0,0))
               for y in range(im.height) for x in range(im.width)),path
    return im.getchannel('A')


def points(im):
    return {(x,y) for y in range(im.height) for x in range(im.width) if im.getpixel((x,y))}


def sha(im):
    return hashlib.sha256(im.tobytes()).hexdigest()


def native_idle_matches(role,frames):
    header=ROOT/'src/ui'/('BirdSprite.h' if role[2]=='bird' else 'WyvernSprite.h')
    body=re.search('const uint8_t '+role[5]+r'\[.*?=\s*\{(.*?)\n\};',header.read_text(),re.S).group(1)
    data=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})',body)]
    assert len(data)==704
    for index,im in enumerate(frames):
        assert all(bool(im.getpixel((x,y))) == (not (data[index*352+y*8+x//8] & (128>>(x%8))))
                   for y in range(44) for x in range(64)),role[0]


rendered={};metadata=[]
for role in ROLES:
    key,label,species,stage,hearts,_=role
    path=ROOT/f'assets/pets/{species}/{stage}'
    idle=read_alpha(path/'idle.png');happy=read_alpha(path/'happy.png')
    assert idle.size==(64,88) and happy.size==(64,44)
    idle_frames=[idle.crop((0,44*i,64,44*(i+1))) for i in range(2)]
    native_idle_matches(role,idle_frames)
    happy_body=happy.copy();heart_parts=[]
    for box in hearts:
        heart_parts.append((box,happy.crop(box)))
        ImageDraw.Draw(happy_body).rectangle((box[0],box[1],box[2]-1,box[3]-1),fill=0)
    # All user changes, after isolating hearts, really are confined to the face.
    face_box=ImageChops.difference(happy_body,idle_frames[0]).getbbox()
    assert face_box and 18<=face_box[0]<face_box[2]<=43 and 18<=face_box[1]<face_box[3]<=30,(key,face_box)
    entry={'role':key,'face_box_frame0_local_exclusive':face_box,'states':[],
           'idle_source_matches_firmware':True,'extra_body_translation':[0,0]}
    for index,original in enumerate(idle_frames):
        face_dy=index if species=='bird' else 0
        body=original.copy()
        body.paste(happy_body.crop(face_box),(face_box[0],face_box[1]+face_dy))
        # Preserve every original moving wing, tail, crest and body pixel.
        outside=ImageChops.difference(body,original)
        ImageDraw.Draw(outside).rectangle((face_box[0],face_box[1]+face_dy,face_box[2]-1,face_box[3]+face_dy-1),fill=0)
        assert not outside.getbbox()
        if index==0:assert body.tobytes()==happy_body.tobytes()
        else:assert body.tobytes()!=happy_body.tobytes(),'Idle motion disappeared'
        rendered[(key,f'idle-{index}')]=original
        body.save(OUT/f'{key}-happy-idle-pose-{index}-64x44.png')
        for heart_phase in (0,1):
            composed=body.copy();heart_points=set()
            for box,part in heart_parts:
                heart_points|={(box[0]+x,box[1]+y-heart_phase) for x,y in points(part)}
            body_points=points(body)
            assert not body_points & heart_points,(key,index,heart_phase)
            gap=min(max(abs(x-a),abs(y-b))-1 for x,y in heart_points for a,b in body_points)
            assert gap>=2,(key,index,heart_phase,gap)
            for point in heart_points:composed.putpixel(point,255)
            assert all(20<=x+32<=108 and 0<=y+6<=54 for x,y in points(composed))
            state=f'happy-{index}-heart-{heart_phase}'
            rendered[(key,state)]=composed
            entry['states'].append({'state':state,'idle_frame':index,'face_shift_y':face_dy,
                                    'heart_shift_y':-heart_phase,'heart_blank_clearance':gap})
    metadata.append(entry)

# Actual UI/U8g2 Home supplies HUD, fonts and glyph pixel semantics.
adapter=(ROOT/'test/rps_ui_native.cpp').read_text(encoding='utf-8').split('static void capture(')[0]
cpp=adapter+r'''
static void saveFrame(const char* name){
 char path[240];snprintf(path,sizeof(path),".pio/touch-happy-idle-v4/%s.pbm",name);
 FILE* f=fopen(path,"wb");assert(f);fprintf(f,"P1\n128 64\n");
 for(int y=0;y<64;++y){for(int x=0;x<128;++x)fprintf(f,"%u ",(pixels[(y/8)*128+x]>>(y%8))&1);fputc(10,f);}fclose(f);
}
int main(){
 info.tile_width=16;info.tile_height=8;info.pixel_width=128;info.pixel_height=64;
 gfx.u8x8.display_info=&info;u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);u8g2_SetDrawColor(&gfx,1);
'''
for role in ROLES:
    key,label,species,stage,*_=role
    cpp+='{Hardware::Display display;Hardware::Sound sound;Pet::PetData pet;Storage::Memorials album;\n'
    cpp+=f'assert(pet.startNewEgg(1,"Tamama",Pet::SpeciesId::{species.title()}));\n'
    cpp+=f'auto s=pet.snapshot();s.lifeStage=Pet::LifeStage::{stage.title()};s.ageSeconds=Pet::PetData::k{"EggHatch" if stage=="baby" else "Adult"}AgeSeconds;assert(pet.restore(s));\n'
    cpp+='Ui::UiController ui(display,sound,pet,album,randomMove);ui.init(0);ui.update(Hardware::InputEvent::None,2000);ui.update(Hardware::InputEvent::None,2600);ui.render();auto base=pixels;\n'
    cpp+=f'saveFrame("{key}-base");\n'
    for (name,state),art in rendered.items():
        if name!=key:continue
        cpp+='{pixels=base;display.clearArea(20,0,89,55);\n'
        for x,y in sorted(points(art)):cpp+=f'display.drawLine({32+x},{6+y},{32+x},{6+y});\n'
        cpp+=f'saveFrame("{key}-{state}");}}\n'
    cpp+='}\n'
cpp+='}\n'
(WORK/'preview.cpp').write_text(cpp,encoding='utf-8')
clib=ROOT/'.pio/libdeps/esp32dev/U8g2/src/clib'
objects=list((ROOT/'.pio/rps-preview').glob('u8*.o'));assert objects
subprocess.run(['g++','-std=c++11','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
 '-Itest/support','-Isrc','-Iinclude','-I'+str(clib),str(WORK/'preview.cpp'),
 'src/ui/UiController.cpp','src/ui/PetIcons.cpp','src/games/RpsGame.cpp','src/games/MemoryNotes.cpp',
 'src/pet/PetData.cpp','src/pet/PetSnapshot.cpp','src/storage/Memorials.cpp',*map(str,objects),'-o',str(WORK/'preview.exe')],cwd=ROOT,check=True)
subprocess.run([str(WORK/'preview.exe')],cwd=ROOT,check=True)


def load(key,state):
    return ImageChops.invert(Image.open(WORK/f'{key}-{state}.pbm').convert('L'))


for (key,state),expected in rendered.items():
    actual=load(key,state);base=load(key,'base')
    assert actual.crop((32,6,96,50)).tobytes()==expected.tobytes()
    for box in [(0,0,20,64),(109,0,128,64),(0,55,128,64)]:
        assert actual.crop(box).tobytes()==base.crop(box).tobytes()
    actual.save(OUT/f'{key}-{state}-128x64.png')

END=5650;PET_START=700;HOLD_END=4700;PET_END=4950
DRAGON_EDGES=(170,330,500,670,830)


def body_index(species,t):
    if species=='bird':return int(t%2600>=1600)
    # GIF uses 10ms ticks; round 1/6s boundaries within 3.4ms.
    return sum(t%1000>=edge for edge in DRAGON_EDGES)%2


def at(role,t):
    key,_,species,*_=role
    index=body_index(species,t)
    if PET_START<=t<PET_END:
        heart_phase=((min(t,HOLD_END)-PET_START)//400)%2
        return load(key,f'happy-{index}-heart-{heart_phase}')
    return load(key,f'idle-{index}')


def event_times(species):
    times={0,END,PET_START,HOLD_END,PET_END}
    times.update(range(PET_START,HOLD_END+1,400))
    if species=='bird':
        times.update(t for start in range(0,END,2600) for t in (start,start+1600) if t<END)
    else:
        times.update(t for start in range(0,END,1000) for t in (start,*[start+e for e in DRAGON_EDGES]) if t<END)
    return sorted(times)


def save_gif(path,frames,times):
    durations=[b-a for a,b in zip(times,times[1:])]
    frames[0].save(path,save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)
    expected=[];ms=[]
    for frame,duration in zip(frames,durations):
        if expected and expected[-1].tobytes()==frame.tobytes():ms[-1]+=duration
        else:expected.append(frame);ms.append(duration)
    gif=Image.open(path);assert gif.n_frames==len(expected)
    for i,frame in enumerate(expected):
        gif.seek(i);assert gif.convert('RGB').tobytes()==frame.convert('RGB').tobytes()
        assert gif.info['duration']==ms[i]


for role,entry in zip(ROLES,metadata):
    key=role[0];times=event_times(role[2])
    for scale in (1,4):
        frames=[at(role,t).resize((128*scale,64*scale),Image.Resampling.NEAREST) for t in times[:-1]]
        save_gif(OUT/f'{key}-happy-idle-{128*scale}x{64*scale}.gif',frames,times)
    entry['timeline_ms']=times
    entry['idle_cycle_ms']=2600 if role[2]=='bird' else 1000
    entry['idle_frame_timing_ms']=[1600,1000] if role[2]=='bird' else [170,160,170,170,160,170]

font=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',19)
sheet=Image.new('RGB',(1080,650),'#171b21');draw=ImageDraw.Draw(sheet)
draw.text((16,10),'摸頭 V4｜真正待機兩幀＋使用者笑臉｜愛心獨立輕飄・4 倍最近鄰',font=font,fill='white')
for i,role in enumerate(ROLES):
    x=16+(i%2)*536;y=42+(i//2)*300
    draw.text((x,y),role[1]+('｜原待機 1.6s / 1s' if role[2]=='bird' else '｜原待機約 6 FPS'),font=font,fill='white')
    sheet.paste(load(role[0],'happy-0-heart-0').convert('RGB').resize((512,256),Image.Resampling.NEAREST),(x,y+30))
sheet.save(OUT/'happy-idle-contact.png')
second=sheet.copy()
for i,role in enumerate(ROLES):
    x=16+(i%2)*536;y=42+(i//2)*300+30
    second.paste(load(role[0],'happy-1-heart-1').convert('RGB').resize((512,256),Image.Resampling.NEAREST),(x,y))
second.save(OUT/'happy-idle-second-pose.png')
times=sorted(set(event_times('bird'))|set(event_times('wyvern')))
frames=[]
for t in times[:-1]:
    frame=sheet.copy()
    for i,role in enumerate(ROLES):
        frame.paste(at(role,t).convert('RGB').resize((512,256),Image.Resampling.NEAREST),(16+(i%2)*536,72+(i//2)*300))
    frames.append(frame)
# Verify every decoded OLED cell; palette quantization may change label antialiasing.
frames[0].save(OUT/'happy-idle-contact.gif',save_all=True,append_images=frames[1:],
               duration=[b-a for a,b in zip(times,times[1:])],loop=0,disposal=2)
gif=Image.open(OUT/'happy-idle-contact.gif');assert gif.n_frames==len(frames)
for i,t in enumerate(times[:-1]):
    gif.seek(i);assert gif.info['duration']==times[i+1]-t
    for n,role in enumerate(ROLES):
        x=16+(n%2)*536;y=72+(n//2)*300
        assert gif.convert('L').crop((x,y,x+512,y+256)).tobytes()==at(role,t).resize((512,256),Image.Resampling.NEAREST).tobytes()
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==value for p,value in SOURCE_HASHES.items())
manifest={'status':'HAPPY_IDLE_V4_PREVIEW_ONLY','roles':metadata,'source_sha256':SOURCE_HASHES,
 'validated_native_states':len(rendered),'pet_start_ms':PET_START,'hold_end_ms':HOLD_END,
 'tail_end_ms':PET_END,'loop_duration_ms':END,'gif_files_verified':9,
 'firmware_source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in
 ['src/ui/UiController.cpp','src/ui/PetIcons.cpp','src/ui/BirdSprite.h','src/ui/WyvernSprite.h','test/rps_ui_native.cpp']}}
(OUT/'validation.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: 24 native states, original idle outside face, happy face tracking, independent hearts, HUD/EXP, 9 GIF files verified.')
print(OUT/'happy-idle-contact.gif')
