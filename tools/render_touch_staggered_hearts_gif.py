"""Encode the approved V5 timing proposal as seamless GIF loops; no art edits."""
from pathlib import Path
from math import lcm
import hashlib
import json
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/previews/touch-proposal/user-art-v5-staggered'
V4=ROOT/'docs/previews/touch-proposal/user-art-v4'
DATA=json.loads((OUT/'validation.json').read_text(encoding='utf-8'))
PERIOD=DATA['period_ms'];VISIBLE=DATA['visible_ms'];START=2000


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def lit(im):
    return {(x,y) for y in range(im.height) for x in range(im.width) if im.getpixel((x,y))}


def phase(t,delay):
    if t<delay:return None
    age=(t-delay)%PERIOD
    if age>=VISIBLE:return None
    return -1 if 100<=age<500 or 900<=age<1300 else 0


assert all(digest(ROOT/p)==sha for p,sha in DATA['source_sha256'].items())
assert all(digest(ROOT/p)==sha for p,sha in DATA['v4_files_sha256'].items())
results=[]
for role in DATA['roles']:
    key=role['role'];species,stage=key.split('-')
    delays=[heart['delay_ms'] for heart in role['hearts']]
    happy=Image.open(ROOT/f'assets/pets/{species}/{stage}/happy.png').getchannel('A')
    hearts=[]
    for heart in role['hearts']:
        box=heart['v4_local_rectangle_exclusive']
        hearts.append({(32+box[0]+x,6+box[1]+y) for x,y in lit(happy.crop(box))})
    allowed=set().union(*(h|{(x,y-1) for x,y in h} for h in hearts))
    cache={}
    for pose in (0,1):
        reference=Image.open(V4/f'{key}-happy-{pose}-heart-0-128x64.png').convert('L')
        for a in (None,0,-1):
            for b in (None,0,-1):
                im=reference.copy()
                for p in set.union(*hearts):im.putpixel(p,0)
                for index,dy in enumerate((a,b)):
                    if dy is not None:
                        for x,y in hearts[index]:im.putpixel((x,y+dy),255)
                assert all(im.getpixel((x,y))==reference.getpixel((x,y))
                           for y in range(64) for x in range(128) if (x,y) not in allowed)
                cache[(pose,a,b)]=im

    def frame_at(t):
        now=t+DATA['body_phase_origin_ms']
        pose=int(now%2600>=1600) if species=='bird' else sum(now%1000>=e for e in (170,330,500,670,830))%2
        return cache[(pose,*[phase(t,d) for d in delays])]

    # Use exact common periods, so neither body nor heart clocks jump on repeat.
    duration=lcm(PERIOD,2600 if species=='bird' else 1000);end=START+duration
    times={START,end}
    for delay in delays:
        for start in range(delay,end+PERIOD,PERIOD):
            times.update(start+age for age in (0,100,500,900,1300,VISIBLE) if START<start+age<end)
    body_cycle=2600 if species=='bird' else 1000
    edges=(0,1600) if species=='bird' else (0,170,330,500,670,830)
    for base in range(0,end+body_cycle+700,body_cycle):
        times.update(base+edge-700 for edge in edges if START<base+edge-700<end)
    times=sorted(times)
    assert frame_at(START).tobytes()==frame_at(end).tobytes()
    # Every frame in the supplied PNG sequence must still match this encoder.
    for item in role['frames']:
        assert Image.open(OUT/item['png']).convert('L').tobytes()==frame_at(item['time_ms']).tobytes()
    frames=[];durations=[]
    for a,b in zip(times,times[1:]):
        im=frame_at(a)
        if frames and frames[-1].tobytes()==im.tobytes():durations[-1]+=b-a
        else:frames.append(im);durations.append(b-a)
    for scale in (1,4):
        enlarged=[im.resize((128*scale,64*scale),Image.Resampling.NEAREST) for im in frames]
        path=OUT/f'{key}-staggered-{128*scale}x{64*scale}.gif'
        enlarged[0].save(path,save_all=True,append_images=enlarged[1:],duration=durations,loop=0,disposal=2)
        gif=Image.open(path);assert gif.n_frames==len(enlarged)
        for i,expected in enumerate(enlarged):
            gif.seek(i);assert gif.convert('L').tobytes()==expected.tobytes()
            assert gif.info['duration']==durations[i]
        results.append({'file':path.name,'frame_count':len(enlarged),'loop_duration_ms':duration,
                        'seamless_body_and_heart_phase':True,'sha256':digest(path)})
assert all(digest(ROOT/p)==sha for p,sha in DATA['source_sha256'].items())
assert all(digest(ROOT/p)==sha for p,sha in DATA['v4_files_sha256'].items())
(OUT/'gif-validation.json').write_text(json.dumps({'source':'validation.json','steady_start_ms':START,
 'png_frames_unchanged':True,'gif_files':results},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: 8 GIFs; exact V5 PNG pixels and timings; seamless body/heart phases; V4/source art unchanged.')
