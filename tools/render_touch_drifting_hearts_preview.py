"""V7 preview: independent 90ms XY overlays over preserved V4 framebuffers."""
from pathlib import Path
from math import ceil, lcm
import argparse
import hashlib
import json
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
V4=ROOT/'docs/previews/touch-proposal/user-art-v4'
V5=ROOT/'docs/previews/touch-proposal/user-art-v5-staggered'
V6=ROOT/'docs/previews/touch-proposal/user-art-v6-floating'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--no-hidden',action='store_true',help='V8: immediately respawn at 310ms without a hidden frame')
ARGS=parser.parse_args()
VERSION='V8' if ARGS.no_hidden else 'V7'
OUT=ROOT/('docs/previews/touch-proposal/user-art-v8-no-hidden' if ARGS.no_hidden else 'docs/previews/touch-proposal/user-art-v7-drifting')
OUT.mkdir(parents=True,exist_ok=True)
DATA=json.loads((V5/'validation.json').read_text(encoding='utf-8'))
LABELS={'bird-baby':'幼鳥','bird-adult':'成鳥','wyvern-baby':'幼龍','wyvern-adult':'成龍'}
# H0/H1 keep their original anchors, masks and side assignment.
# Per-character pairs intentionally differ; every visible step changes X and Y.
PATHS={
 'bird-baby':   [[(0,0),(-1,-1),(-2,-2),(-1,-3)],[(0,0),(1,-1),(2,-2),(1,-3)]],
 'bird-adult':  [[(0,0),(-1,-1),(0,-2),(-1,-3)],[(0,0),(1,-1),(2,-2),(1,-3)]],
 'wyvern-baby': [[(0,0),(-1,-1),(-2,-2),(-1,-3)],[(0,0),(1,-1),(0,-2),(1,-3)]],
 'wyvern-adult':[[(0,0),(-1,-1),(0,-2),(-1,-3)],[(0,0),(1,-1),(0,-2),(1,-3)]],
}
STEPS=(0,40,130,220);EXIT=310;PERIOD=310 if ARGS.no_hidden else 400;END=1600
FONT=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',16)
TITLE=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',20)


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def lit(im):return {(x,y) for y in range(im.height) for x in range(im.width) if im.getpixel((x,y))}


def phase(t,delay):
    if t<delay:return None
    age=(t-delay)%PERIOD
    if age>=EXIT:return None
    return next(i for i in range(3,-1,-1) if age>=STEPS[i])


def body_at(species,t):
    now=t+DATA['body_phase_origin_ms']
    return int(now%2600>=1600) if species=='bird' else sum(now%1000>=e for e in (170,330,500,670,830))%2


def event_times(species,delays,start,end):
    times={start,end}
    for delay in delays:
        for begin in range(delay,end+PERIOD,PERIOD):
            times.update(begin+age for age in (*STEPS,EXIT) if start<begin+age<end)
    cycle=2600 if species=='bird' else 1000
    edges=(0,1600) if species=='bird' else (0,170,330,500,670,830)
    for begin in range(0,end+cycle+700,cycle):
        times.update(begin+edge-700 for edge in edges if start<begin+edge-700<end)
    return sorted(times)


def save_gif(path,images,times,loop):
    frames=[];durations=[]
    for im,a,b in zip(images,times,times[1:]):
        if frames and frames[-1].tobytes()==im.tobytes():durations[-1]+=b-a
        else:frames.append(im);durations.append(b-a)
    options={'loop':0} if loop else {}
    frames[0].save(path,save_all=True,append_images=frames[1:],duration=durations,disposal=2,**options)
    gif=Image.open(path);assert gif.n_frames==len(frames)
    for i,im in enumerate(frames):
        gif.seek(i);assert gif.convert('L').tobytes()==im.tobytes()
        assert gif.info['duration']==durations[i]
    return {'file':path.name,'frames':len(frames),'duration_ms':sum(durations),'loop':loop,
            'pixels_and_durations_verified':True,'sha256':digest(path)}


def state_data(key,t,delays):
    return [{'id':f'H{i}','age_ms':None if t<d else (t-d)%PERIOD,
             'phase':phase(t,d),'xy':None if phase(t,d) is None else PATHS[key][i][phase(t,d)]}
            for i,d in enumerate(delays)]


assert all(digest(ROOT/p)==sha for p,sha in DATA['source_sha256'].items())
preserved={p.relative_to(ROOT).as_posix():digest(p) for folder in (V4,V5,V6) for p in folder.rglob('*') if p.is_file()}
if ARGS.no_hidden:
    previous=ROOT/'docs/previews/touch-proposal/user-art-v7-drifting'
    preserved.update({p.relative_to(ROOT).as_posix():digest(p) for p in previous.rglob('*') if p.is_file()})
firmware=json.loads((V4/'validation.json').read_text(encoding='utf-8'))['firmware_source_sha256']
assert all(digest(ROOT/p)==sha for p,sha in firmware.items())
report={'status':f'XY_DRIFT_{VERSION}_PREVIEW_ONLY','period_ms':PERIOD,'visible_ms':EXIT,'hidden_ms':PERIOD-EXIT,
 'instant_rebirth':ARGS.no_hidden,
 'step_times_ms':STEPS,'overlay_independent_of_body_fps':True,'source_sha256':DATA['source_sha256'],
 'preserved_v4_v5_v6_sha256':preserved,'firmware_source_sha256':firmware,'roles':[],'gif_files':[]}
trajectory=Image.new('RGB',(1104,44+8*200),'#171b21')
ImageDraw.Draw(trajectory).text((12,8),f'愛心 {VERSION}｜出生40ms，之後每90ms同時變更 X/Y｜'+('310ms直接重生' if ARGS.no_hidden else '座標皆相對原基準'),font=TITLE,fill='white')
png_total=0;contexts=[]
for row,role in enumerate(DATA['roles']):
    key=role['role'];species,stage=key.split('-');delays=[h['delay_ms'] for h in role['hearts']]
    happy=Image.open(ROOT/f'assets/pets/{species}/{stage}/happy.png').getchannel('A')
    hearts=[]
    for h in role['hearts']:
        patch=happy.crop(h['v4_local_rectangle_exclusive'])
        assert hashlib.sha256(patch.tobytes()).hexdigest()==h['mask_sha256']
        hearts.append({(h['v4_oled_origin'][0]+x,h['v4_oled_origin'][1]+y) for x,y in lit(patch)})
    for path in PATHS[key]:
        assert path[0]==(0,0) and path[-1][1]==-3
        assert all(abs(x)<=2 and -3<=y<=0 for x,y in path)
        assert all(a[0]!=b[0] and b[1]<a[1] for a,b in zip(path,path[1:]))
    allowed=set().union(*({(x+dx,y+dy) for x,y in h for dx,dy in PATHS[key][i]} for i,h in enumerate(hearts)))
    cache={}
    for pose in (0,1):
        reference=Image.open(V4/f'{key}-happy-{pose}-heart-0-128x64.png').convert('L')
        body=Image.open(V4/f'{key}-happy-idle-pose-{pose}-64x44.png').convert('L')
        body_pixels={(32+x,6+y) for x,y in lit(body)}
        for a in (None,0,1,2,3):
            for b in (None,0,1,2,3):
                im=reference.copy();drawn=set()
                for p in set.union(*hearts):im.putpixel(p,0)
                for i,index in enumerate((a,b)):
                    if index is None:continue
                    dx,dy=PATHS[key][i][index]
                    pixels={(x+dx,y+dy) for x,y in hearts[i]}
                    assert len(pixels)==len(hearts[i]) and not pixels & (body_pixels|drawn)
                    assert all(20<=x<=108 and 0<=y<=54 for x,y in pixels)
                    drawn|=pixels
                    for p in pixels:im.putpixel(p,255)
                assert all(im.getpixel((x,y))==reference.getpixel((x,y))
                           for y in range(64) for x in range(128) if (x,y) not in allowed)
                # All body pixels stay visible, and no additional character pixels appear.
                assert lit(im.crop((20,0,109,55)))=={(x-20,y) for x,y in body_pixels|drawn}
                if (a,b)==(0,0):assert im.tobytes()==reference.tobytes()
                cache[(pose,a,b)]=im

    def frame_at(t,species=species,delays=delays,cache=cache):
        return cache[(body_at(species,t),*[phase(t,d) for d in delays])]

    # Every own update changes both axes, irrespective of the other heart/body.
    for i,delay in enumerate(delays):
        assert phase(delay+39,delay)==0 and phase(delay+40,delay)==1
        for cycle in range(4):
            begin=delay+cycle*PERIOD
            previous=None
            for t in range(begin,begin+EXIT):
                xy=PATHS[key][i][phase(t,delay)]
                if previous is not None:assert xy[1]<=previous[1]
                previous=xy
            if ARGS.no_hidden:
                assert phase(begin+EXIT-1,delay)==3
                assert phase(begin+EXIT,delay)==0
                assert all(phase(t,delay) is not None for t in range(begin,begin+PERIOD))
            else:
                assert phase(begin+EXIT,delay) is None
                assert phase(begin+PERIOD-1,delay) is None
            assert phase(begin+PERIOD,delay)==0
    for t in range(4*PERIOD+max(delays)):
        assert any(phase(t,d) is not None for d in delays)
    times=event_times(species,delays,0,END)
    # Explicitly prove overlay movement occurs while the character pose is unchanged.
    assert any(body_at(species,t-1)==body_at(species,t) and frame_at(t-1).tobytes()!=frame_at(t).tobytes()
               for t in times if t>0)
    folder=OUT/key;folder.mkdir(exist_ok=True)
    entry={'role':key,'hearts':[],'png_frames':[],'every_visible_step_changes_x_and_y':True,
           'no_simultaneous_blank':True,'overlay_moves_with_unchanged_body_pose':True}
    for i,h in enumerate(role['hearts']):
        entry['hearts'].append({**h,'first_float_ms':h['delay_ms']+40,
          'first_disappear_ms':None if ARGS.no_hidden else h['delay_ms']+EXIT,
          'first_reappear_ms':h['delay_ms']+PERIOD,'hidden_after_birth':not ARGS.no_hidden,
          'path_relative_to_anchor':PATHS[key][i], 'frame_times_ms':STEPS,
          'allowed_displacements':PATHS[key][i]})
        for index,(age,(dx,dy)) in enumerate(zip(STEPS,PATHS[key][i])):
            t=h['delay_ms']+age;im=frame_at(t);x=12+index*276;y=44+(row*2+i)*200
            d=ImageDraw.Draw(trajectory)
            d.text((x,y),f'{LABELS[key]} H{i} F{index} ({dx:+d},{dy:+d})',font=FONT,fill='white')
            trajectory.paste(im.convert('RGB').resize((256,128),Image.Resampling.NEAREST),(x,y+24))
            d.text((x,y+156),f'自身 {age}ms／摸頭 {t}ms',font=FONT,fill='white')
    strip=Image.new('RGB',(1104,44+ceil(len(times)/4)*185),'#171b21')
    ImageDraw.Draw(strip).text((12,8),LABELS[key]+'｜所有 overlay 與待機更新節點',font=TITLE,fill='white')
    for index,t in enumerate(times):
        im=frame_at(t);name=f'frame-{index:03d}-{t:04d}ms'
        im.save(folder/(name+'-128x64.png'))
        im.resize((512,256),Image.Resampling.NEAREST).save(folder/(name+'-512x256.png'))
        assert Image.open(folder/(name+'-128x64.png')).tobytes()==im.tobytes()
        x=12+(index%4)*276;y=44+(index//4)*185
        ImageDraw.Draw(strip).text((x,y),f'{t:04d}ms',font=FONT,fill='white')
        strip.paste(im.convert('RGB').resize((256,128),Image.Resampling.NEAREST),(x,y+24))
        entry['png_frames'].append({'time_ms':t,'body_pose':body_at(species,t),
             'hearts':state_data(key,t,delays),'png':f'{key}/{name}-128x64.png'})
    strip.save(OUT/f'{key}-all-frames.png')
    for mode,start,duration,loop in [('entry',0,END,False),('loop',2000,lcm(PERIOD,2600 if species=='bird' else 1000),True)]:
        ticks=event_times(species,delays,start,start+duration)
        if loop:assert frame_at(start).tobytes()==frame_at(start+duration).tobytes()
        for scale in (1,4):
            frames=[frame_at(t).resize((128*scale,64*scale),Image.Resampling.NEAREST) for t in ticks[:-1]]
            report['gif_files'].append(save_gif(OUT/f'{key}-drifting-{mode}-{128*scale}x{64*scale}.gif',frames,ticks,loop))
    report['roles'].append(entry);contexts.append((key,frame_at));png_total+=len(times)
trajectory.save(OUT/'relative-xy-trajectories.png')
# All-four start demonstration; played once so birth clocks are not reset on repeat.
ticks=sorted(set(t['time_ms'] for role in report['roles'] for t in role['png_frames']))
background=Image.new('RGB',(1080,650),'#171b21');d=ImageDraw.Draw(background)
d.text((16,10),f'摸頭 {VERSION}｜出生40ms、每90ms斜向漂移｜'+('最高點直接回低處重生，無隱藏等待' if ARGS.no_hidden else '獨立 overlay・原角色與座標不變'),font=TITLE,fill='white')
for i,(key,_) in enumerate(contexts):d.text((16+(i%2)*536,42+(i//2)*300),LABELS[key],font=TITLE,fill='white')
frames=[]
for t in ticks[:-1]:
    im=background.copy()
    for i,(_,at) in enumerate(contexts):
        im.paste(at(t).convert('RGB').resize((512,256),Image.Resampling.NEAREST),(16+(i%2)*536,72+(i//2)*300))
    frames.append(im)
frames[0].save(OUT/'drifting-entry-overview.gif',save_all=True,append_images=frames[1:],
 duration=[b-a for a,b in zip(ticks,ticks[1:])],disposal=2)
gif=Image.open(OUT/'drifting-entry-overview.gif');assert gif.n_frames==len(frames)
for i,t in enumerate(ticks[:-1]):
    gif.seek(i);assert gif.info['duration']==ticks[i+1]-t
    for j,(_,at) in enumerate(contexts):
        x=16+(j%2)*536;y=72+(j//2)*300
        assert gif.convert('L').crop((x,y,x+512,y+256)).tobytes()==at(t).resize((512,256),Image.Resampling.NEAREST).tobytes()
report['gif_files'].append({'file':'drifting-entry-overview.gif','loop':False,'pixels_and_durations_verified':True})
assert all(digest(ROOT/p)==sha for p,sha in DATA['source_sha256'].items())
assert all(digest(ROOT/p)==sha for p,sha in preserved.items())
assert all(digest(ROOT/p)==sha for p,sha in firmware.items())
report['png_timepoints']=png_total
(OUT/'validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
lines=[f'# 摸頭愛心 {VERSION}：40ms 出生＋90ms XY 漂移'+('，直接重生' if ARGS.no_hidden else '')+'（待審核）','',
 '原基準點出生只停 40ms，接著在 40／130／220ms 三個節點同時改變 X、Y；'+('310ms 最高點直接切回低處重生，不插入隱藏幀。每輪310ms。' if ARGS.no_hidden else '310ms 從高處消失，隱藏90ms，400ms從低處重生。')+'每步向上1px，整段上飄3px，水平偏移最多2px。',
 '', '先後／時間差不變：幼鳥左先右+200ms、成鳥右先左+180ms、幼龍左先右+220ms、成龍右先左+160ms。基準點、愛心形狀／大小／左右配置、閉眼表情、角色與UI原像素完全沿用。','',
 '## 相對基準座標的逐幀路徑','']
for key,paths in PATHS.items():
    lines += [f'- {LABELS[key]}左 H0：'+' → '.join(str(p) for p in paths[0])+'；右 H1：'+' → '.join(str(p) for p in paths[1])+'。']
lines += ['', '![四角色各愛心逐幀軌跡](relative-xy-trajectories.png)','',
 '## GIF 預覽','']
for key,_ in contexts:
    lines += [f'- {LABELS[key]}：[循環4倍]({key}-drifting-loop-512x256.gif)・[循環原尺寸]({key}-drifting-loop-128x64.gif)・[首次生成]({key}-drifting-entry-512x256.gif)・[全部逐幀PNG]({key}-all-frames.png)。']
lines += ['', '![四角色首次生成總覽，播放一次](drifting-entry-overview.gif)','',
 '維持 V7 獨立 overlay，位移節點不等待角色換幀。小鳥1600/1000ms、小飛龍約6FPS的原待機與閉眼表情不變。正式整合時，overlay狀態變更應獨立設 dirty_，讓UI可在角色同一幀內重繪；本輪未改正式UiController。',
 '',f'驗證：{png_total}個PNG時間點、17個GIF回讀；每步X/Y都有位移、Y只上升、先後及獨立循環、無同步全空白、所有軌跡與兩種角色姿勢無重疊／裁切、HUD／角色像素保留。循環GIF使用共同週期，重播相位連續。舊V4/V5/V6與來源PNG、正式韌體雜湊未變。',
 '', '重建：有Pillow的Python執行 `tools/render_touch_drifting_hearts_preview.py'+(' --no-hidden' if ARGS.no_hidden else '')+'`。詳見 [validation.json](validation.json)。這是預覽，尚未正式實作或實機驗收。','']
(OUT/'README.md').write_text('\n'.join(lines),encoding='utf-8')
print(f'PASS: {png_total} PNG timepoints, 17 GIFs; 40ms birth, 90ms XY steps, independent overlay, art/body/HUD preserved.')
print(OUT/'relative-xy-trajectories.png')
