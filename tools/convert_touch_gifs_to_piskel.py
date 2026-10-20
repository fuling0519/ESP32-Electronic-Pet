"""Export the four V8 OLED GIFs as lossless, fixed-FPS Piskel v2 projects."""
from base64 import b64decode, b64encode
from functools import reduce
from hashlib import sha256
from io import BytesIO
import json
from math import gcd
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'docs/previews/touch-proposal/user-art-v8-no-hidden'
OUT = SOURCE / 'piskel'
ROLES = {'bird-baby': '幼鳥', 'bird-adult': '成鳥',
         'wyvern-baby': '幼龍', 'wyvern-adult': '成龍'}


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def read_gif(path):
    frames, durations = [], []
    with Image.open(path) as gif:
        assert gif.info.get('loop') == 0
        for i in range(gif.n_frames):
            gif.seek(i)
            frames.append(gif.convert('RGBA'))
            durations.append(gif.info['duration'])
    return frames, durations


def convert(role):
    source = SOURCE / f'{role}-drifting-loop-128x64.gif'
    enlarged = SOURCE / f'{role}-drifting-loop-512x256.gif'
    hashes = {p.name: digest(p) for p in (source, enlarged)}
    frames, durations = read_gif(source)
    large_frames, large_durations = read_gif(enlarged)
    assert durations == large_durations
    for small, large in zip(frames, large_frames):
        assert small.resize(large.size, Image.Resampling.NEAREST).tobytes() == large.tobytes()
    del large_frames

    tick = reduce(gcd, durations)
    assert tick > 0 and 1000 % tick == 0
    # Repeat identical frames instead of rounding variable GIF durations.
    timeline = []
    mapping = []
    for index, duration in enumerate(durations):
        count = duration // tick
        mapping.append({'gif_frame': index, 'duration_ms': duration,
                        'piskel_first_frame': len(timeline), 'piskel_frame_count': count})
        timeline.extend([index] * count)

    chunks = []
    for offset in range(0, len(timeline), 256):
        indexes = timeline[offset:offset + 256]
        # Official layout[x][y] maps a spritesheet cell to its timeline index.
        # Divisors avoid unused cells / invalid frame indexes in the final chunk.
        columns = next(n for n in range(min(16, len(indexes)), 0, -1)
                       if len(indexes) % n == 0)
        rows = len(indexes) // columns
        sheet = Image.new('RGBA', (128 * columns, 64 * rows))
        layout = [[0] * rows for _ in range(columns)]
        for local, source_index in enumerate(indexes):
            x, y = local % columns, local // columns
            sheet.paste(frames[source_index], (x * 128, y * 64))
            layout[x][y] = offset + local
        png = BytesIO()
        sheet.save(png, format='PNG', optimize=True)
        chunks.append({'layout': layout,
                       'base64PNG': 'data:image/png;base64,' + b64encode(png.getvalue()).decode('ascii')})

    layer = {'name': 'OLED - original GIF pixels', 'opacity': 1,
             'frameCount': len(timeline), 'chunks': chunks}
    project = {'modelVersion': 2, 'piskel': {
        'name': f'{role} - petting V8',
        'description': '128x64 OLED V8 loop. Exact GIF timing via repeated frames; '
                       f'{1000 // tick} FPS, one flattened layer. Do not change FPS to retain timing.',
        'fps': 1000 // tick, 'width': 128, 'height': 64,
        'layers': [json.dumps(layer, separators=(',', ':'))], 'hiddenFrames': []}}
    target = OUT / f'{role}-petting-v8-128x64.piskel'
    target.write_text(json.dumps(project, separators=(',', ':')), encoding='utf-8')

    # Read the saved file, independently decode every chunk and compare each
    # timeline slot with the fully composited GIF frame, including alpha.
    saved = json.loads(target.read_text(encoding='utf-8'))
    assert saved['modelVersion'] == 2
    data = saved['piskel']
    decoded_layer = json.loads(data['layers'][0])
    seen = set()
    for chunk in decoded_layer['chunks']:
        sheet = Image.open(BytesIO(b64decode(chunk['base64PNG'].split(',', 1)[1]))).convert('RGBA')
        columns, rows = len(chunk['layout']), len(chunk['layout'][0])
        assert sheet.size == (128 * columns, 64 * rows)
        for x, column in enumerate(chunk['layout']):
            assert len(column) == rows
            for y, index in enumerate(column):
                assert index not in seen and 0 <= index < len(timeline)
                seen.add(index)
                restored = sheet.crop((x * 128, y * 64, (x + 1) * 128, (y + 1) * 64))
                assert restored.tobytes() == frames[timeline[index]].tobytes()
    assert seen == set(range(len(timeline)))
    assert decoded_layer['frameCount'] * 1000 == sum(durations) * data['fps']
    for p in (source, enlarged):
        assert hashes[p.name] == digest(p)
    return {'role': role, 'file': target.name, 'sha256': digest(target),
            'source_sha256': hashes, 'source_frames': len(frames),
            'piskel_frames': len(timeline), 'fps': data['fps'],
            'duration_ms': sum(durations), 'size': [128, 64],
            'layers': 1, 'all_pixels_and_timing_exact': True,
            'matches_4x_preview_exactly': True, 'source_frame_mapping': mapping,
            'editor_import_tested': False}


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    results = [convert(role) for role in ROLES]
    (OUT / 'validation.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    readme = '''# 摸頭愛心 V8：Piskel 編輯檔

四個 .piskel 對應 V8 的四組連續循環 GIF，使用原生 128×64 像素。
原生畫面放大四倍後，與前次提供的 512×256 GIF 每幀像素完全一致。
角色、閉眼表情、愛心、UI 均未變更；由 GIF 轉換，所以只有一個合併圖層。

在 Piskel 的 Import / Browse .piskel files 選取檔案開啟。

## 時間與編輯注意事項

Piskel 專案只有一個全域 FPS，不能直接儲存每幀各自的時長。
因此這份轉換使用 100 FPS（每格 10ms），以重複幀完整保留原 GIF 時間軸，
沒有丟幀或四捨五入。幼鳥、成鳥各 8060 格／80.6 秒；幼龍、成龍各 3100 格／31 秒。
連續相同畫面是原動畫的停留時間，不是新增動作。

Piskel 網頁的 FPS 滑桿上限為 24，而檔案格式／控制器可讀取數值 100。
修改滑桿會改變整體播放速度；編輯器／瀏覽器也可能無法即時播放 100 FPS。
這些檔案以像素編輯與精確時間資料保存為主，觀賞節奏仍請比對原 GIF。
已完成檔案回讀、每一時間格 RGBA 像素及總時長比對；尚未在 Piskel UI 實際匯入驗收。

格式依據（Piskel 官方原始碼）：
- [Serializer](https://github.com/piskelapp/piskel/blob/master/src/js/utils/serialization/Serializer.js)
- [Deserializer](https://github.com/piskelapp/piskel/blob/master/src/js/utils/serialization/Deserializer.js)
- [FrameUtils / layout](https://github.com/piskelapp/piskel/blob/master/src/js/utils/FrameUtils.js)
- [FPS controller](https://github.com/piskelapp/piskel/blob/master/src/js/controller/piskel/PiskelController.js)
- [FPS slider](https://github.com/piskelapp/piskel/blob/master/src/templates/preview.html)

來源 GIF、使用者素材及正式韌體未修改。這次只做格式轉換。
'''
    (OUT / 'README.md').write_text(readme, encoding='utf-8')
    archive = OUT / 'petting-v8-piskel-all.zip'
    with ZipFile(archive, 'w', ZIP_DEFLATED) as bundle:
        for path in sorted(OUT.glob('*.piskel')):
            bundle.write(path, path.name)
        for name in ('README.md', 'validation.json'):
            bundle.write(OUT / name, name)
    for result in results:
        print({k: v for k, v in result.items() if k not in ('source_frame_mapping', 'source_sha256')})
    print(f'ZIP: {archive} ({archive.stat().st_size} bytes)')


if __name__ == '__main__':
    main()
