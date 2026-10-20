"""V5 PNG-only timing proposal over byte-preserved V4 OLED framebuffers.

Uses V4 heart coordinates, masks, body poses and HUD verbatim. Does not rerun
or write the V4 renderer, source PNGs, sprite headers or production firmware.
"""
from pathlib import Path
import ast
import hashlib
import json
from math import ceil
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
V4=ROOT/'docs/previews/touch-proposal/user-art-v4'
OUT=ROOT/'docs/previews/touch-proposal/user-art-v5-staggered'
OUT.mkdir(parents=True,exist_ok=True)
tree=ast.parse((ROOT/'tools/render_touch_happy_idle_preview.py').read_text(encoding='utf-8'))
ROLES=ast.literal_eval(next(node.value for node in tree.body if isinstance(node,ast.Assign)
                         and any(isinstance(t,ast.Name) and t.id=='ROLES' for t in node.targets)))
# Explicit per-character timing; order is not inferred from size or position.
OFFSETS={'bird-baby':(0,200),'bird-adult':(180,0),
         'wyvern-baby':(0,220),'wyvern-adult':(160,0)}
PERIOD=1800;VISIBLE=1680;END=4000
PHASES=(0,100,500,900,1300,1680)
V4_META=json.loads((V4/'validation.json').read_text(encoding='utf-8'))
assert V4_META['pet_start_ms']==700
FONT=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',16)
TITLE=ImageFont.truetype('C:/Windows/Fonts/msjh.ttc',20)


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def lit(im):
    return {(x,y) for y in range(im.height) for x in range(im.width) if im.getpixel((x,y))}


def clock(t,delay):
    if t<delay:return {'visible':False,'age_ms':None,'cycle':None,'dy':None}
    age=(t-delay)%PERIOD
    visible=age<VISIBLE
    dy=-(1 if 100<=age<500 or 900<=age<1300 else 0) if visible else None
    return {'visible':visible,'age_ms':age,'cycle':(t-delay)//PERIOD,'dy':dy}


def body_index(role,t):
    # Same global body phase as V4 at petting start, no pose/timing change.
    now=t+V4_META['pet_start_ms']
    if role[2]=='bird':return int(now%2600>=1600)
    return sum(now%1000>=edge for edge in (170,330,500,670,830))%2


def times_for(role):
    times={0,END}
    for delay in OFFSETS[role[0]]:
        for start in range(delay,END+PERIOD,PERIOD):
            times.update(start+age for age in PHASES if 0<=start+age<END)
    previous=body_index(role,0)
    for t in range(1,END):
        current=body_index(role,t)
        if current!=previous:times.add(t)
        previous=current
    return sorted(times)


def text_clock(c):
    return '隱藏' if not c['visible'] else ('原位' if c['dy']==0 else '上浮1px')


def card(canvas,xy,role,t,frame,label=''):
    x,y=xy;d=ImageDraw.Draw(canvas)
    d.text((x,y),f'{role[1]}｜{t:04d}ms'+('・'+label if label else ''),font=FONT,fill='white')
    canvas.paste(frame.convert('RGB').resize((256,128),Image.Resampling.NEAREST),(x,y+24))
    c=[clock(t,delay) for delay in OFFSETS[role[0]]]
    d.text((x,y+156),f'H0 {text_clock(c[0])}／H1 {text_clock(c[1])}',font=FONT,fill='white')


source_hashes={p:digest(ROOT/p) for p in V4_META['source_sha256']}
assert source_hashes==V4_META['source_sha256'],'Source art changed after V4; review required'
preserved_files={p.relative_to(ROOT).as_posix():digest(p) for p in V4.iterdir() if p.is_file()}
overview=Image.new('RGB',(1104,860),'#171b21')
boundary=overview.copy()
ImageDraw.Draw(overview).text((12,8),'愛心 V5｜出現順序分鏡｜H0／H1 沿用 V4 配置，不交換位置',font=TITLE,fill='white')
ImageDraw.Draw(boundary).text((12,8),'愛心 V5｜消失與循環交界｜每顆獨立計時，另一顆持續顯示',font=TITLE,fill='white')
manifest={'status':'STAGGERED_HEARTS_V5_PNG_PROPOSAL_ONLY','period_ms':PERIOD,
 'visible_ms':VISIBLE,'hidden_ms':PERIOD-VISIBLE,'preview_end_ms':END,
 'body_phase_origin_ms':V4_META['pet_start_ms'],'source_sha256':source_hashes,
 'v4_files_sha256':preserved_files,'roles':[]}
frame_total=0
for row,role in enumerate(ROLES):
    key,label,species,stage,boxes,_=role
    delays=OFFSETS[key];delta=max(delays)
    folder=OUT/key;folder.mkdir(exist_ok=True)
    happy=Image.open(ROOT/f'assets/pets/{species}/{stage}/happy.png').convert('RGBA').getchannel('A')
    hearts=[]
    for index,box in enumerate(boxes):
        # Crop the existing rectangle literally; no bbox search/recentering.
        patch=happy.crop(box)
        pixels={(32+box[0]+x,6+box[1]+y) for x,y in lit(patch)}
        hearts.append(pixels)
    allowed=set().union(*(h|{(x,y-1) for x,y in h} for h in hearts))
    cache={};refs=[];bodies=[]
    for pose in (0,1):
        reference=Image.open(V4/f'{key}-happy-{pose}-heart-0-128x64.png').convert('L')
        body=Image.open(V4/f'{key}-happy-idle-pose-{pose}-64x44.png').convert('L')
        body_points={(x+32,y+6) for x,y in lit(body)}
        # The saved V4 body and heart masks must exactly match its real buffer.
        assert not body_points & set.union(*hearts)
        expected=body_points|set.union(*hearts)
        assert lit(reference.crop((20,0,109,55)))=={(x-20,y) for x,y in expected}
        for states in ((a,b) for a in (None,0,-1) for b in (None,0,-1)):
            im=reference.copy()
            for point in set.union(*hearts):im.putpixel(point,0)
            for index,dy in enumerate(states):
                if dy is None:continue
                shifted={(x,y+dy) for x,y in hearts[index]}
                assert not shifted & body_points
                for point in shifted:im.putpixel(point,255)
            assert all(im.getpixel((x,y))==reference.getpixel((x,y))
                       for y in range(64) for x in range(128) if (x,y) not in allowed)
            # Restore both at dy=0 and obtain byte-identical V4 pixels.
            if states==(0,0):assert im.tobytes()==reference.tobytes()
            cache[(pose,*states)]=im
        refs.append(reference);bodies.append(body)

    def frame_at(t):
        c=[clock(t,delay) for delay in delays]
        return cache[(body_index(role,t),*[state['dy'] if state['visible'] else None for state in c])]

    # Millisecond-level audit through three complete independent heart cycles.
    for t in range(3*PERIOD+delta):
        assert any(clock(t,delay)['visible'] for delay in delays),(key,'both hidden',t)
        if t>=delta:
            for delay in delays:
                a=clock(t,delay);b=clock(t+PERIOD,delay)
                assert (a['visible'],a['dy'])==(b['visible'],b['dy'])
    first=delays.index(0);second=1-first
    assert clock(delta,0)['dy']==-1 and clock(delta,delta)['dy']==0
    assert not clock(VISIBLE,0)['visible'] and clock(VISIBLE,delta)['visible']
    assert not clock(VISIBLE+delta,delta)['visible'] and clock(VISIBLE+delta,0)['visible']
    assert any(clock(t,delays[0])['dy']!=clock(t,delays[1])['dy'] for t in range(END))
    timeline=times_for(role)
    entry={'role':key,'hearts':[],'frames':[],'frame_directory':key,
           'all_body_and_hud_pixels_identical_to_v4':True,'no_both_hidden_1ms_audit':True}
    for index,(box,pixels,delay) in enumerate(zip(boxes,hearts,delays)):
        entry['hearts'].append({'id':f'H{index}','v4_local_rectangle_exclusive':box,
             'v4_oled_origin':[32+box[0],6+box[1]],'white_pixel_count':len(pixels),
             'delay_ms':delay,'first_float_ms':delay+100,'first_disappear_ms':delay+VISIBLE,
             'first_reappear_ms':delay+PERIOD,'allowed_displacements':[[0,0],[0,-1]],
             'mask_sha256':hashlib.sha256(bytes(happy.crop(box).tobytes())).hexdigest()})
    strip=Image.new('RGB',(1104,44+ceil(len(timeline)/4)*200),'#171b21')
    ImageDraw.Draw(strip).text((12,8),f'{label}｜0–4000ms 全部畫面變化幀｜H0 +{delays[0]}／H1 +{delays[1]}ms',font=TITLE,fill='white')
    for i,t in enumerate(timeline):
        im=frame_at(t)
        name=f'frame-{i:03d}-{t:04d}ms'
        im.save(folder/(name+'-128x64.png'))
        im.resize((512,256),Image.Resampling.NEAREST).save(folder/(name+'-512x256.png'))
        assert Image.open(folder/(name+'-128x64.png')).convert('L').tobytes()==im.tobytes()
        card(strip,(12+(i%4)*276,44+(i//4)*200),role,t,im)
        entry['frames'].append({'index':i,'time_ms':t,
             'duration_to_next_ms':timeline[i+1]-t if i+1<len(timeline) else None,
             'body_pose':body_index(role,t),'hearts':[clock(t,d) for d in delays],
             'png':f'{key}/{name}-128x64.png','sha256':hashlib.sha256(im.tobytes()).hexdigest()})
    strip.save(OUT/f'{key}-all-frames.png')
    for col,t in enumerate((0,100,delta,delta+100)):
        card(overview,(12+col*276,44+row*200),role,t,frame_at(t))
    for col,t in enumerate((VISIBLE,PERIOD,VISIBLE+delta,PERIOD+delta)):
        card(boundary,(12+col*276,44+row*200),role,t,frame_at(t))
    entry['first_visible_heart']=f'H{first}';entry['second_visible_heart']=f'H{second}'
    frame_total+=len(timeline);manifest['roles'].append(entry)
overview.save(OUT/'appearance-sequence.png');boundary.save(OUT/'cycle-boundaries.png')
assert all(digest(ROOT/p)==value for p,value in source_hashes.items())
assert all(digest(ROOT/p)==value for p,value in preserved_files.items())
manifest['total_png_frames_native']=frame_total
(OUT/'validation.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
notes=['# 摸頭愛心 V5：時間錯位 PNG 提案（2026-10-20）','',
 '只調整 V4 愛心時間，角色閉眼表情、真實待機動作、HUD、愛心像素／大小／初始座標全部沿用。尚未修改正式韌體，未新增實機驗收。',
 '', '## 時間安排','',
 '- 幼鳥：H0 左小愛心 0ms；H1 右大愛心 +200ms。',
 '- 成鳥：H1 右大愛心 0ms；H0 左小愛心 +180ms。',
 '- 幼龍：H0 左大愛心 0ms；H1 右小愛心 +220ms。',
 '- 成龍：H1 右小愛心 0ms；H0 左大愛心 +160ms。','',
 'H0／H1 僅是 V4 配置中的固定識別碼，不按大小或左右排序決定誰先出現；這次沒有交換兩顆愛心。0ms 是開始摸頭回饋的時間，角色的待機相位沿用 V4。','',
 '每顆各自從出現時刻開始計時：0–99ms 原位；100–499ms 上移 1px；500–899ms 原位；900–1299ms 上移 1px；1300–1679ms 原位；1680–1799ms 隱藏，1800ms 開始下一輪。保持原有兩個位置，沒有新增位移、縮放、旋轉或粒子。',
 '', '後出者的時鐘獨立延遲 160／180／200／220ms，並在後續每輪保持這個時間差。第一顆在 100ms 開始上浮，第二顆之後才從原定位出現；隱藏各 120ms 的區間互不重疊，因此連續摸頭沒有兩顆同步清空。單顆在隱藏期間回到初始相位，另一顆繼續自身進度。','',
 '## 分鏡 PNG','',
 '![出現分鏡](appearance-sequence.png)','',
 '![消失與循環交界](cycle-boundaries.png)','',
 '## 全部逐幀 PNG','',
 f'共 {frame_total} 個時間點，每個有 128×64 原尺寸及 512×256 最近鄰版本。包含 0–4000ms 所有愛心／身體幀變化事件及終點；各圖標示的 H0／H1 狀態與時間只在比較分鏡中，不畫進 OLED。','']
for role,entry in zip(ROLES,manifest['roles']):
    notes += [f'### {role[1]}','',f'![全部畫面變化幀]({role[0]}-all-frames.png)','',
              '原尺寸獨立 PNG：','']
    notes += [f'- [{f["time_ms"]}ms]({f["png"]})・[4 倍]({f["png"].replace("128x64","512x256")})' for f in entry['frames']]
    notes += ['']
notes += ['## GIF 循環預覽','',
 'PNG 的 V5 時間安排亦可用 `tools/render_touch_staggered_hearts_gif.py` 匯出 GIF。採已開始摸頭後的連續循環片段，不在 GIF 重播時同時重設兩顆愛心；小鳥 23.4 秒、小飛龍 9 秒，皆為愛心與待機動作的共同週期。','']
for role in ROLES:
    notes += [f'- {role[1]}：[4 倍]({role[0]}-staggered-512x256.gif)・[原尺寸]({role[0]}-staggered-128x64.gif)。']
notes += ['','## 驗證界線','',
 '沿用先前真實 U8g2 的 V4 framebuffer。新圖僅在原有愛心像素及既有上移 1px 的位置範圍變更；全部其他像素與同一待機姿勢的 V4 畫面相同。兩顆同在原位時可逐位元還原 V4。原 PNG、V4 所有產物及正式韌體未改動。',
 '', '逐毫秒檢查連續三輪：無兩顆同時隱藏、獨立相位循環一致、先上浮再出現第二顆、消失錯開。PNG 回讀核對通過；詳細原座標／遮罩雜湊、每幀狀態及來源雜湊見 [validation.json](validation.json)。這是待審核的時間提案，不是正式觸摸功能或 OLED 實機驗收。',
 '', '重建 PNG：以有 Pillow 的 Python 執行 `tools/render_touch_staggered_hearts_preview.py`。GIF 另由 `tools/render_touch_staggered_hearts_gif.py` 匯出；兩者均不執行 V4 renderer、不修改正式 UI。','']
(OUT/'README.md').write_text('\n'.join(notes),encoding='utf-8')
print(f'PASS: {frame_total} PNG timepoints, independent clocks, no blank heart-cycle seam; exact V4 body/HUD/masks/anchors preserved.')
print(OUT/'appearance-sequence.png');print(OUT/'cycle-boundaries.png')
