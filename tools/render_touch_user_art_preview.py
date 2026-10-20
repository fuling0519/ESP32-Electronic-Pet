"""Preview-only shock/happy sheets over the unchanged native U8g2 Home.

Keeps user PNGs and firmware untouched. Run with Pillow after test_rps.ps1.
"""
from pathlib import Path
import hashlib
import json
import subprocess
from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/previews/touch-proposal/user-art-v3'
WORK = ROOT / '.pio/touch-user-art-v3'
OUT.mkdir(parents=True, exist_ok=True)
WORK.mkdir(parents=True, exist_ok=True)
ROLES = [
    # Heart rectangles are exclusive local sprite coordinates, read from user art.
    ('bird-baby', '幼鳥', 'bird', 'baby', [(13,14,18,19), (44,11,51,17)]),
    ('bird-adult', '成鳥', 'bird', 'adult', [(6,12,11,17), (43,8,50,14)]),
    ('wyvern-baby', '幼龍', 'wyvern', 'baby', [(12,9,19,15), (48,14,53,19)]),
    ('wyvern-adult', '成龍', 'wyvern', 'adult', [(5,19,12,25), (55,10,60,15)]),
]


def points(im):
    return {(x,y) for y in range(im.height) for x in range(im.width) if im.getpixel((x,y))}


def read_frames(path):
    im = Image.open(path).convert('RGBA')
    assert im.width % 64 == 0 and im.height % 44 == 0, (path,im.size)
    assert all(a in (0,255) and (not a or (r,g,b)==(255,255,255))
               for r,g,b,a in (im.getpixel((x,y)) for y in range(im.height) for x in range(im.width))), path
    frames=[]
    for y in range(0,im.height,44):
        for x in range(0,im.width,64):
            frames.append(im.crop((x,y,x+64,y+44)).getchannel('A'))
    return frames


def legacy_offset(role,kind,phase):
    baby=role[3]=='baby'; dragon=role[2]=='wyvern'
    if kind=='happy': return (0,1 if dragon and not baby else phase)
    if baby: return (phase if dragon else 0,-2 if phase==0 else -1)
    return (1 if phase==0 else 0,0)


def animation(role,kind,count):
    if kind=='happy':
        # One supplied expression keeps the earlier two-phase 400ms motion.
        return [(i % count,400,*legacy_offset(role,kind,i%2),-(i%2))
                for i in range(max(2,count))]
    if count==1:
        return [(0,duration,*legacy_offset(role,kind,i),0)
                for i,duration in enumerate((160,340))]
    if count==2:
        # Like adults, hold the shock expression through both motion phases.
        # The second supplied frame is reserved for recovery after all 500ms.
        return [(0,160,1,0,0), (0,340,0,0,0)]
    # General row-major sheet support; display every supplied frame within 500ms.
    units=50//count
    assert units>0, 'Too many shock frames for a readable 500ms reaction'
    return [(i,(units if i<count-1 else 50-units*(count-1))*10,0,0,0)
            for i in range(count)]


def compose(role,kind,art,dx,dy,heart_dy):
    body=art.copy(); heart_layer=Image.new('L',(64,44)); hearts=[]
    if kind=='happy':
        for box in role[4]:
            crop=art.crop(box)
            assert crop.getbbox(), ('Empty heart anchor',role[0],box)
            heart_layer.paste(crop,box[:2])
            ImageDraw.Draw(body).rectangle((box[0],box[1],box[2]-1,box[3]-1),fill=0)
            hearts.append((box,crop))
        assert ImageChops.lighter(body,heart_layer).tobytes()==art.tobytes()
    screen=Image.new('L',(128,64))
    screen.paste(body,(32+dx,6+dy))
    body_pixels=points(screen)
    heart_pixels=set()
    for box,crop in hearts:
        heart_pixels |= {(32+box[0]+x,6+box[1]+heart_dy+y) for x,y in points(crop)}
    assert not body_pixels & heart_pixels, (role[0],kind,'heart overlaps character')
    for p in heart_pixels: screen.putpixel(p,255)
    assert all(20<=x<=108 and 0<=y<=54 for x,y in points(screen)), role[0]
    gap=min((max(abs(x-a),abs(y-b))-1 for x,y in heart_pixels for a,b in body_pixels),default=None)
    return screen,gap


manifest={'status':'USER_ART_V3_PREVIEW_ONLY_NOT_FIRMWARE_OR_HARDWARE_ACCEPTANCE','roles':[]}
rendered={}
source_hashes={}
for role in ROLES:
    key,label,species,stage,boxes=role
    entry={'role':key,'expressions':{}}
    for kind in ('shock','happy'):
        path=ROOT/f'assets/pets/{species}/{stage}/{kind}.png'
        source_hashes[str(path.relative_to(ROOT))]=hashlib.sha256(path.read_bytes()).hexdigest()
        arts=read_frames(path)
        schedule=animation(role,kind,len(arts))
        frames=[]; details=[]
        for i,(index,ms,dx,dy,heart_dy) in enumerate(schedule):
            screen,gap=compose(role,kind,arts[index],dx,dy,heart_dy)
            frames.append(screen)
            details.append({'source_frame':index,'duration_ms':ms,'body_offset':[dx,dy],
                            'heart_offset':[0,heart_dy], 'heart_clear_blank_pixels':gap})
        rendered[(key,kind)]=frames
        entry['expressions'][kind]={'source':str(path.relative_to(ROOT)).replace('\\','/'),
                                    'source_frame_count':len(arts),'schedule':details}
        if kind=='shock' and stage=='baby':
            entry['expressions'][kind]['recovery_source_frame']=1
            entry['expressions'][kind]['recovery_body_offset']=[0,0]
            rendered[(key,'recovered')]=[compose(role,kind,arts[1],0,0,0)[0]]
    manifest['roles'].append(entry)

# Use the same host adapter, Display glyph rasterizer and real UI as native tests.
adapter=(ROOT/'test/rps_ui_native.cpp').read_text(encoding='utf-8').split('static void capture(')[0]
cpp=adapter+r'''
static void saveFrame(const char* name) {
 char path[240]; snprintf(path,sizeof(path),".pio/touch-user-art-v3/%s.pbm",name);
 FILE* f=fopen(path,"wb"); assert(f); fprintf(f,"P1\n128 64\n");
 for(int y=0;y<64;++y){for(int x=0;x<128;++x) fprintf(f,"%u ",(pixels[(y/8)*128+x]>>(y%8))&1);fputc(10,f);}fclose(f);
}
int main(){
 info.tile_width=16; info.tile_height=8; info.pixel_width=128; info.pixel_height=64;
 gfx.u8x8.display_info=&info;
 u8g2_SetupBuffer(&gfx,pixels.data(),8,u8g2_ll_hvline_vertical_top_lsb,U8G2_R0);
 u8g2_SetDrawColor(&gfx,1);
'''
for role,entry in zip(ROLES,manifest['roles']):
    key,label,species,stage,_=role
    cpp+=' { Hardware::Display display; Hardware::Sound sound; Pet::PetData pet; Storage::Memorials album;\n'
    cpp+=f'assert(pet.startNewEgg(1,"Tamama",Pet::SpeciesId::{species.title()}));\n'
    cpp+=f'auto s=pet.snapshot(); s.lifeStage=Pet::LifeStage::{stage.title()}; s.ageSeconds=Pet::PetData::k{"EggHatch" if stage=="baby" else "Adult"}AgeSeconds; assert(pet.restore(s));\n'
    cpp+='Ui::UiController ui(display,sound,pet,album,randomMove);ui.init(0);ui.update(Hardware::InputEvent::None,2000);ui.update(Hardware::InputEvent::None,2600);ui.render();\n'
    cpp+=f'saveFrame("{key}-base"); auto base=pixels;\n'
    cpp+=f'ui.update(Hardware::InputEvent::Right,2601);ui.render();saveFrame("{key}-focus");\n'
    for kind in ('shock','happy','recovered'):
        if (key,kind) not in rendered: continue
        for i,frame in enumerate(rendered[(key,kind)]):
            # Rasterize the already-composed user glyphs with the real Display adapter.
            # Include hearts outside the 64px art canvas after body motion.
            cpp+='{ pixels=base; display.clearArea(20,0,89,55);\n'
            for x,y in sorted(points(frame)):
                cpp+=f'display.drawLine({x},{y},{x},{y});\n'
            cpp+=f'saveFrame("{key}-{kind}-{i}"); }}\n'
    cpp+=' }\n'
cpp+='}\n'
(WORK/'preview.cpp').write_text(cpp,encoding='utf-8')
clib=ROOT/'.pio/libdeps/esp32dev/U8g2/src/clib'
objects=list((ROOT/'.pio/rps-preview').glob('u8*.o'))
assert objects,'Run tools/test_rps.ps1 first'
subprocess.run(['g++','-std=c++11','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
    '-Itest/support','-Isrc','-Iinclude','-I'+str(clib),str(WORK/'preview.cpp'),
    'src/ui/UiController.cpp','src/ui/PetIcons.cpp','src/games/RpsGame.cpp','src/games/MemoryNotes.cpp',
    'src/pet/PetData.cpp','src/pet/PetSnapshot.cpp','src/storage/Memorials.cpp',
    *map(str,objects),'-o',str(WORK/'preview.exe')],cwd=ROOT,check=True)
subprocess.run([str(WORK/'preview.exe')],cwd=ROOT,check=True)


def load(name):
    return ImageChops.invert(Image.open(WORK/(name+'.pbm')).convert('L'))


def rest_image(key,kind):
    # The juvenile shock's second supplied frame is also its recovered picture.
    if kind=='shock' and key.endswith('-baby'):
        return load(key+'-recovered-0')
    return load(key+'-base')


font=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',20)
sheet=Image.new('RGB',(1080,1298),'#171b21');draw=ImageDraw.Draw(sheet)
draw.text((16,10),'使用者原圖 V3｜幼年戳戳改為成年節奏・恢復使用第二幀｜其餘沿用 V2',font=font,fill='white')
frame_total=0
for row,(role,entry) in enumerate(zip(ROLES,manifest['roles'])):
    key,label,*_=role
    base=load(key+'-base'); focus=load(key+'-focus')
    base.save(OUT/(key+'-base-128x64.png'))
    assert base.crop((0,0,109,64)).tobytes()==focus.crop((0,0,109,64)).tobytes()
    for col,kind in enumerate(('shock','happy')):
        frames=rendered[(key,kind)]
        details=entry['expressions'][kind]
        for i,expected in enumerate(frames):
            actual=load(f'{key}-{kind}-{i}')
            assert set(actual.tobytes())<={0,255}
            assert actual.crop((20,0,109,55)).tobytes()==expected.crop((20,0,109,55)).tobytes()
            for box in [(0,0,20,64),(109,0,128,64),(0,55,128,64)]:
                assert actual.crop(box).tobytes()==base.crop(box).tobytes(),(key,kind,box)
            actual.save(OUT/f'{key}-{kind}-{i}-128x64.png')
            actual.resize((512,256),Image.Resampling.NEAREST).save(OUT/f'{key}-{kind}-{i}-512x256.png')
            details['schedule'][i]['rendered_sha256']=hashlib.sha256(actual.tobytes()).hexdigest()
            frame_total+=1
        x,y=16+col*536,58+row*310
        n=details['source_frame_count']
        draw.text((x,y),f'{label}｜{kind}・原圖 {n} 幀',font=font,fill='white')
        sheet.paste(load(f'{key}-{kind}-0').convert('RGB').resize((512,256),Image.Resampling.NEAREST),(x,y+32))
        names=[f'{key}-{kind}-{i}' for i in range(len(frames))]
        durations=[s['duration_ms'] for s in details['schedule']]
        if kind=='happy':
            names=names*2+[names[0]];durations=durations*2+[250]
        rest=rest_image(key,kind)
        if kind=='shock' and key.endswith('-baby'):
            rest.save(OUT/(key+'-shock-recovered-128x64.png'))
            assert rest.crop((32,6,96,50)).tobytes()==read_frames(ROOT/details['source'])[1].tobytes()
            expected=rendered[(key,'recovered')][0]
            assert rest.crop((20,0,109,55)).tobytes()==expected.crop((20,0,109,55)).tobytes()
            for box in [(0,0,20,64),(109,0,128,64),(0,55,128,64)]:
                assert rest.crop(box).tobytes()==base.crop(box).tobytes()
        timeline=[rest]+[load(name) for name in names]+[rest]
        times=[700]+durations+[700]
        for scale in (1,4):
            images=[im.resize((128*scale,64*scale),Image.Resampling.NEAREST) for im in timeline]
            path=OUT/f'{key}-{kind}-{128*scale}x{64*scale}.gif'
            images[0].save(path,save_all=True,append_images=images[1:],duration=times,loop=0,disposal=2)
            # GIF encoders coalesce adjacent identical frames (the supplied
            # baby-wyvern settle frame happens to equal the current idle).
            expected_frames=[];expected_times=[]
            for im,ms in zip(images,times):
                if expected_frames and expected_frames[-1].tobytes()==im.tobytes():
                    expected_times[-1]+=ms
                else:
                    expected_frames.append(im);expected_times.append(ms)
            gif=Image.open(path);assert gif.n_frames==len(expected_frames),(path,gif.n_frames)
            for i,im in enumerate(expected_frames):
                gif.seek(i);assert gif.convert('L').tobytes()==im.tobytes()
                assert gif.info['duration']==expected_times[i]
sheet.save(OUT/'contact-sheet.png')
# Synchronized overview: each cell retains its own actual reaction timings.
def cell_at(key,kind,t,entry):
    if t<700: return rest_image(key,kind)
    schedule=entry['expressions'][kind]['schedule']
    indices=list(range(len(schedule)));durations=[p['duration_ms'] for p in schedule]
    if kind=='happy': indices=indices*2+[0];durations=durations*2+[250]
    elapsed=t-700
    for index,ms in zip(indices,durations):
        if elapsed<ms:return load(f'{key}-{kind}-{index}')
        elapsed-=ms
    return rest_image(key,kind)

times={0,3250}
for entry in manifest['roles']:
    for kind in ('shock','happy'):
        schedule=entry['expressions'][kind]['schedule']
        durations=[p['duration_ms'] for p in schedule]
        if kind=='happy':durations=durations*2+[250]
        elapsed=700;times.add(elapsed)
        for ms in durations:elapsed+=ms;times.add(elapsed)
times=sorted(times)
overview=[]
for t in times[:-1]:
    canvas=sheet.copy()
    for row,(role,entry) in enumerate(zip(ROLES,manifest['roles'])):
        for col,kind in enumerate(('shock','happy')):
            im=cell_at(role[0],kind,t,entry)
            canvas.paste(im.convert('RGB').resize((512,256),Image.Resampling.NEAREST),
                         (16+col*536,58+row*310+32))
    overview.append(canvas)
overview[0].save(OUT/'contact-sheet.gif',save_all=True,append_images=overview[1:],
                 duration=[b-a for a,b in zip(times,times[1:])],loop=0,disposal=2)
overview_gif=Image.open(OUT/'contact-sheet.gif')
assert overview_gif.n_frames==len(overview)
for i,t in enumerate(times[:-1]):
    overview_gif.seek(i)
    assert overview_gif.info['duration']==times[i+1]-t
    for row,(role,entry) in enumerate(zip(ROLES,manifest['roles'])):
        for col,kind in enumerate(('shock','happy')):
            x,y=16+col*536,58+row*310+32
            expected=cell_at(role[0],kind,t,entry).resize((512,256),Image.Resampling.NEAREST)
            actual=overview_gif.convert('L').crop((x,y,x+512,y+256))
            assert actual.tobytes()==expected.tobytes()
# All other reaction phases form a storyboard for reviewing supplied multi-frame art.
story_height=38+sum((len(e['expressions']['shock']['schedule'])+1)//2*292 for e in manifest['roles'])
story=Image.new('RGB',(1080,story_height),'#171b21');d=ImageDraw.Draw(story)
d.text((16,5),'戳戳 shock 動作分鏡｜依播放順序，左右再下一列｜不重畫原圖',font=font,fill='white')
top=38
for role,entry in zip(ROLES,manifest['roles']):
    key,label,*_=role
    schedule=entry['expressions']['shock']['schedule']
    for i,item in enumerate(schedule):
        x=16+(i%2)*536;y=top+(i//2)*292
        d.text((x,y),f'{label}｜第 {item["source_frame"]+1} 幀・{item["duration_ms"]}ms',font=font,fill='white')
        story.paste(load(f'{key}-shock-{i}').convert('RGB').resize((512,256),Image.Resampling.NEAREST),(x,y+30))
    top+=((len(schedule)+1)//2)*292
story.save(OUT/'shock-storyboard.png')
manifest['validated_reaction_frames']=frame_total
manifest['validated_recovery_frames']=2
manifest['overview']={'frame_count':len(overview),'duration_ms':3250,'all_oled_cells_pixel_verified':True}
manifest['source_sha256']=source_hashes
manifest['firmware_source_sha256']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in
    ['src/ui/UiController.cpp','src/ui/PetIcons.cpp','src/ui/BirdSprite.h','src/ui/WyvernSprite.h','test/rps_ui_native.cpp']}
# User source assets must remain byte-identical throughout the preview process.
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==sha for p,sha in source_hashes.items())
(OUT/'validation.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(f'PASS: {frame_total} U8g2 reaction frames, 8 unchanged user sheets, 16 GIFs pixel/timing verified, HUD/footer intact.')
print(OUT/'contact-sheet.png')
