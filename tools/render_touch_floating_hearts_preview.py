"""V6 preview only: independent low-birth -> rise -> high-exit heart cycles.

Reuses V4 actual framebuffer poses and V5 literal heart masks/anchors/order.
Never writes source artwork, previous preview versions or production firmware.
"""
from pathlib import Path
from math import ceil, lcm
import hashlib
import json
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
V4=ROOT/'docs/previews/touch-proposal/user-art-v4'
V5=ROOT/'docs/previews/touch-proposal/user-art-v5-staggered'
OUT=ROOT/'docs/previews/touch-proposal/user-art-v6-floating'
OUT.mkdir(parents=True,exist_ok=True)
DATA=json.loads((V5/'validation.json').read_text(encoding='utf-8'))
LABELS={'bird-baby':'幼鳥','bird-adult':'成鳥','wyvern-baby':'幼龍','wyvern-adult':'成龍'}
PERIOD=1800;VISIBLE=1660;END=4000
PHASES=((0,0),(150,-1),(650,-2),(1150,-3),(1660,None))
FONT=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',16)
TITLE=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',20)


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def lit(im):
    return {(x,y) for y in range(im.height) for x in range(im.width) if im.getpixel((x,y))}


def heart_clock(t,delay):
    if t<delay:return {'visible':False,'age_ms':None,'cycle':None,'dy':None}
    age=(t-delay)%PERIOD
    dy=next(dy for begin,dy in reversed(PHASES) if age>=begin)
    return {'visible':dy is not None,'age_ms':age,'cycle':(t-delay)//PERIOD,'dy':dy}


def pose_at(species,t):
    now=t+DATA['body_phase_origin_ms']
    return int(now%2600>=1600) if species=='bird' else sum(now%1000>=e for e in (170,330,500,670,830))%2


def event_times(species,delays,start,end):
    times={start,end}
    for delay in delays:
        for begin in range(delay,end+PERIOD,PERIOD):
            times.update(begin+age for age,_ in PHASES if start<begin+age<end)
    body_cycle=2600 if species=='bird' else 1000
    edges=(0,1600) if species=='bird' else (0,170,330,500,670,830)
    for begin in range(0,end+body_cycle+700,body_cycle):
        times.update(begin+edge-700 for edge in edges if start<begin+edge-700<end)
    return sorted(times)


def save_gif(path,images,durations,loop):
    frames=[];times=[]
    for im,ms in zip(images,durations):
        if frames and frames[-1].tobytes()==im.tobytes():times[-1]+=ms
        else:frames.append(im);times.append(ms)
    options={'loop':0} if loop else {}
    frames[0].save(path,save_all=True,append_images=frames[1:],duration=times,disposal=2,**options)
    gif=Image.open(path);assert gif.n_frames==len(frames)
    for i,expected in enumerate(frames):
        gif.seek(i);assert gif.convert('L').tobytes()==expected.tobytes()
        assert gif.info['duration']==times[i]
    return {'file':path.name,'frames':len(frames),'duration_ms':sum(times),'loop':loop,
            'pixel_and_duration_verified':True,'sha256':digest(path)}


def clock_text(c):return '隱藏' if not c['visible'] else ('出生' if c['dy']==0 else f'升{-c["dy"]}px')


def card(sheet,x,y,key,t,im,delays):
    d=ImageDraw.Draw(sheet)
    d.text((x,y),f'{LABELS[key]}｜{t:04d}ms',font=FONT,fill='white')
    sheet.paste(im.convert('RGB').resize((256,128),Image.Resampling.NEAREST),(x,y+24))
    states=[clock_text(heart_clock(t,delay)) for delay in delays]
    d.text((x,y+156),f'H0 {states[0]}／H1 {states[1]}',font=FONT,fill='white')


assert all(digest(ROOT/p)==sha for p,sha in DATA['source_sha256'].items())
preserved={p.relative_to(ROOT).as_posix():digest(p) for folder in (V4,V5) for p in folder.rglob('*') if p.is_file()}
firmware=json.loads((V4/'validation.json').read_text(encoding='utf-8'))['firmware_source_sha256']
assert all(digest(ROOT/p)==sha for p,sha in firmware.items())
report={'status':'FLOATING_HEARTS_V6_PREVIEW_ONLY','period_ms':PERIOD,'visible_ms':VISIBLE,
 'hidden_ms':PERIOD-VISIBLE,'path_relative_to_fixed_anchor':[[t,dy] for t,dy in PHASES],
 'horizontal_displacement':0,'body_phase_origin_ms':DATA['body_phase_origin_ms'],
 'source_sha256':DATA['source_sha256'],'preserved_v4_v5_files_sha256':preserved,
 'firmware_source_sha256':firmware,'roles':[],'gif_files':[]}
path_sheet=Image.new('RGB',(1104,860),'#171b21')
life_sheet=path_sheet.copy()
ImageDraw.Draw(path_sheet).text((12,8),'愛心 V6｜低處出生 → 上飄1px → 上飄2px → 上飄3px｜單向離場',font=TITLE,fill='white')
ImageDraw.Draw(life_sheet).text((12,8),'愛心 V6｜高處消失 → 另一顆繼續 → 低處重生｜不改基準座標',font=TITLE,fill='white')
png_total=0;contexts=[]
for row,role in enumerate(DATA['roles']):
    key=role['role'];species,stage=key.split('-')
    delays=[h['delay_ms'] for h in role['hearts']]
    happy=Image.open(ROOT/f'assets/pets/{species}/{stage}/happy.png').getchannel('A')
    hearts=[]
    for h in role['hearts']:
        box=h['v4_local_rectangle_exclusive']
        assert hashlib.sha256(happy.crop(box).tobytes()).hexdigest()==h['mask_sha256']
        hearts.append({(h['v4_oled_origin'][0]+x,h['v4_oled_origin'][1]+y) for x,y in lit(happy.crop(box))})
    allowed=set().union(*({(x,y+dy) for x,y in h for dy in (0,-1,-2,-3)} for h in hearts))
    cache={}
    for pose in (0,1):
        reference=Image.open(V4/f'{key}-happy-{pose}-heart-0-128x64.png').convert('L')
        body=Image.open(V4/f'{key}-happy-idle-pose-{pose}-64x44.png').convert('L')
        body_points={(x+32,y+6) for x,y in lit(body)}
        for a in (None,0,-1,-2,-3):
            for b in (None,0,-1,-2,-3):
                im=reference.copy();drawn=set()
                for p in set.union(*hearts):im.putpixel(p,0)
                for index,dy in enumerate((a,b)):
                    if dy is None:continue
                    shifted={(x,y+dy) for x,y in hearts[index]}
                    assert len(shifted)==len(hearts[index]) and not shifted & (body_points|drawn)
                    assert all(20<=x<=108 and 0<=y<=54 for x,y in shifted)
                    drawn|=shifted
                    for p in shifted:im.putpixel(p,255)
                assert lit(im.crop((32,6,96,50)))=={(x-32,y-6) for x,y in body_points|drawn if 32<=x<96 and 6<=y<50}
                assert all(im.getpixel((x,y))==reference.getpixel((x,y))
                           for y in range(64) for x in range(128) if (x,y) not in allowed)
                if (a,b)==(0,0):assert im.tobytes()==reference.tobytes()
                cache[(pose,a,b)]=im

    def frame_at(t,key=key,species=species,delays=delays,cache=cache):
        return cache[(pose_at(species,t),*[heart_clock(t,d)['dy'] for d in delays])]

    # Exhaustive time audit: never descend while visible; reset only after hiding.
    for delay in delays:
        for start in range(delay,delay+3*PERIOD,PERIOD):
            ys=[heart_clock(t,delay)['dy'] for t in range(start,start+VISIBLE)]
            assert ys[0]==0 and ys[-1]==-3 and all(a>=b for a,b in zip(ys,ys[1:]))
            assert not heart_clock(start+VISIBLE,delay)['visible']
            assert not heart_clock(start+PERIOD-1,delay)['visible']
            assert heart_clock(start+PERIOD,delay)['dy']==0
    for t in range(3*PERIOD+max(delays)):
        assert any(heart_clock(t,d)['visible'] for d in delays),(key,t)
    assert heart_clock(max(delays),0)['dy']==-1
    timeline=event_times(species,delays,0,END)
    folder=OUT/key;folder.mkdir(exist_ok=True)
    strip=Image.new('RGB',(1104,44+ceil(len(timeline)/4)*200),'#171b21')
    ImageDraw.Draw(strip).text((12,8),f'{LABELS[key]}｜全部位移／消失／重生幀｜0–4000ms',font=TITLE,fill='white')
    heart_metadata=[{**h,'first_float_ms':h['delay_ms']+150,
                     'first_disappear_ms':h['delay_ms']+VISIBLE,
                     'first_reappear_ms':h['delay_ms']+PERIOD,
                     'allowed_displacements':[[0,0],[0,-1],[0,-2],[0,-3]]}
                    for h in role['hearts']]
    entry={'role':key,'hearts':heart_metadata,'frames':[],
           'monotonic_rise_during_visibility':True,'no_simultaneous_blank':True,
           'same_heart_masks_anchors_order_and_body_as_v4_v5':True}
    for i,t in enumerate(timeline):
        im=frame_at(t);name=f'frame-{i:03d}-{t:04d}ms'
        im.save(folder/(name+'-128x64.png'))
        im.resize((512,256),Image.Resampling.NEAREST).save(folder/(name+'-512x256.png'))
        assert Image.open(folder/(name+'-128x64.png')).tobytes()==im.tobytes()
        card(strip,12+(i%4)*276,44+(i//4)*200,key,t,im,delays)
        entry['frames'].append({'time_ms':t,'hearts':[heart_clock(t,d) for d in delays],
             'body_pose':pose_at(species,t),'png':f'{key}/{name}-128x64.png'})
    strip.save(OUT/f'{key}-all-frames.png')
    for col,t in enumerate((0,150,650,1150)):
        card(path_sheet,12+col*276,44+row*200,key,t,frame_at(t),delays)
    for col,t in enumerate((VISIBLE-10,VISIBLE,PERIOD-10,PERIOD)):
        card(life_sheet,12+col*276,44+row*200,key,t,frame_at(t),delays)
    # Nonlooping entry clip includes the actual first generation order.
    for scale in (1,4):
        frames=[frame_at(t).resize((128*scale,64*scale),Image.Resampling.NEAREST) for t in timeline[:-1]]
        durations=[b-a for a,b in zip(timeline,timeline[1:])]
        report['gif_files'].append(save_gif(OUT/f'{key}-floating-entry-{128*scale}x{64*scale}.gif',frames,durations,False))
    # Full common period makes the repeating GIF preserve both motion clocks.
    begin=2000;duration=lcm(PERIOD,2600 if species=='bird' else 1000)
    times=event_times(species,delays,begin,begin+duration)
    assert frame_at(begin).tobytes()==frame_at(begin+duration).tobytes()
    for scale in (1,4):
        frames=[frame_at(t).resize((128*scale,64*scale),Image.Resampling.NEAREST) for t in times[:-1]]
        durations=[b-a for a,b in zip(times,times[1:])]
        report['gif_files'].append(save_gif(OUT/f'{key}-floating-loop-{128*scale}x{64*scale}.gif',frames,durations,True))
    contexts.append((role,frame_at));png_total+=len(timeline);report['roles'].append(entry)
path_sheet.save(OUT/'rise-path.png');life_sheet.save(OUT/'high-exit-low-rebirth.png')
# Short combined entry preview makes all four generation orders easy to compare.
times=sorted(set(t for role in report['roles'] for t in [f['time_ms'] for f in role['frames']]))
sheet=Image.new('RGB',(1080,650),'#171b21');d=ImageDraw.Draw(sheet)
d.text((16,10),'摸頭 V6｜各自從低處出生、單向上飄3px後離場｜角色／基準／大小不變',font=TITLE,fill='white')
for i,(role,_) in enumerate(contexts):d.text((16+(i%2)*536,42+(i//2)*300),LABELS[role['role']],font=TITLE,fill='white')
images=[]
for t in times[:-1]:
    im=sheet.copy()
    for i,(_,at) in enumerate(contexts):
        im.paste(at(t).convert('RGB').resize((512,256),Image.Resampling.NEAREST),(16+(i%2)*536,72+(i//2)*300))
    images.append(im)
images[0].save(OUT/'floating-entry-overview.gif',save_all=True,append_images=images[1:],
 duration=[b-a for a,b in zip(times,times[1:])],disposal=2)
gif=Image.open(OUT/'floating-entry-overview.gif');assert gif.n_frames==len(images)
for i,t in enumerate(times[:-1]):
    gif.seek(i);assert gif.info['duration']==times[i+1]-t
    for j,(_,at) in enumerate(contexts):
        x=16+(j%2)*536;y=72+(j//2)*300
        assert gif.convert('L').crop((x,y,x+512,y+256)).tobytes()==at(t).resize((512,256),Image.Resampling.NEAREST).tobytes()
report['gif_files'].append({'file':'floating-entry-overview.gif','loop':False,'duration_ms':END,'oled_cells_verified':True})
assert all(digest(ROOT/p)==sha for p,sha in DATA['source_sha256'].items())
assert all(digest(ROOT/p)==sha for p,sha in preserved.items())
assert all(digest(ROOT/p)==sha for p,sha in firmware.items())
report['png_timepoints']=png_total
(OUT/'validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
lines=['# 摸頭愛心 V6：單向上飄、離場、低處重生（待審核）','',
 '基準座標直接作為較低出生點（dy=0），出生停 150ms → dy=-1 → dy=-2 → dy=-3 → 高處消失，隱藏後才回到 dy=0。沒有在可見期間往下擺動，也沒有水平偏移。',
 '', '時間：0–149ms 出生；150–649ms 上飄1px；650–1149ms 上飄2px；1150–1659ms 上飄3px；1660–1799ms 隱藏；1800ms 低處重新出生。',
 '', '先後不變：幼鳥左先／右+200ms，成鳥右先／左+180ms，幼龍左先／右+220ms，成龍右先／左+160ms。先出者先上飄、先在高處消失；每顆各自循環。',
 '', '![完整出生預覽，播放一次](floating-entry-overview.gif)','',
 '![單向位移分鏡](rise-path.png)','', '![高處消失與低處重生](high-exit-low-rebirth.png)','',
 '## 連續循環 GIF 與全部逐幀 PNG','']
for role in report['roles']:
    key=role['role']
    lines += [f'- {LABELS[key]}：[循環 GIF 4倍]({key}-floating-loop-512x256.gif)・[原尺寸]({key}-floating-loop-128x64.gif)・[完整逐幀 PNG]({key}-all-frames.png)。']
lines += ['',f'共 {png_total} 個 PNG 時間點（各有原尺寸與4倍）。GIF 原尺寸／放大共16檔，另有四角色起始總覽；循環版使用角色與愛心共同週期，重播相位連續。起始版播放一次，保留第一次生成的真實順序。','',
 '角色／閉眼表情／待機／HUD 使用 V4 真實 framebuffer 原像素；愛心遮罩、基準點、大小、左右配置與順序取自 V5，未重新定位。只擴充向上路徑為0/-1/-2/-3px。',
 '', '逐毫秒核對三輪：可見期間 y 單調遞減，只在高處消失，完整隱藏140ms後才回到低處，連續循環無兩顆同步隱藏。全部來源、美術、V4/V5產物及正式韌體雜湊不變；GIF逐幀像素／時長回讀通過。這是預覽，不是觸摸／OLED實機驗收。',
 '', '重建：以有 Pillow 的 Python 執行 `tools/render_touch_floating_hearts_preview.py`。詳見 [validation.json](validation.json)。','']
(OUT/'README.md').write_text('\n'.join(lines),encoding='utf-8')
print(f'PASS: {png_total} PNG timepoints; 17 GIFs; monotonic 3px rise, high exit, hidden wait, low rebirth; V4/V5 art/body/HUD preserved.')
print(OUT/'floating-entry-overview.gif')
