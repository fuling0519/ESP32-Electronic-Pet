"""V9 preview: play the user's complete happy sheets without extra overlays."""
from hashlib import sha256
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'docs/previews/touch-proposal/user-art-v4'
OUT = ROOT / 'docs/previews/touch-proposal/user-art-v9-authored-timing'
ROLES = [('bird-baby', '幼鳥', 5, 14), ('bird-adult', '成鳥', 5, 13),
         ('wyvern-baby', '幼龍', 7, 8), ('wyvern-adult', '成龍', 7, 8)]


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def save_verified(path, frames, durations):
    expected, holds = [], []
    for frame, duration in zip(frames, durations):
        if expected and expected[-1].tobytes() == frame.tobytes():
            holds[-1] += duration
        else:
            expected.append(frame)
            holds.append(duration)
    expected[0].save(path, save_all=True, append_images=expected[1:],
                     duration=holds, loop=0, disposal=2, optimize=False)
    with Image.open(path) as gif:
        assert gif.n_frames == len(expected) and gif.info['loop'] == 0
        for index, (frame, duration) in enumerate(zip(expected, holds)):
            gif.seek(index)
            assert gif.convert('L').tobytes() == frame.tobytes()
            assert gif.info['duration'] == duration
    return {'file': path.name, 'sha256': digest(path),
            'encoded_frames': len(expected), 'duration_ms': sum(holds),
            'all_pixels_and_durations_verified': True}


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    protected = [p for folder in ('src', 'include', 'assets/pets')
                 for p in (ROOT / folder).rglob('*') if p.is_file()]
    protected += [p for p in OUT.parent.rglob('*') if p.is_file() and OUT not in p.parents]
    before = {p.relative_to(ROOT).as_posix(): digest(p) for p in protected}
    report = {'status': 'V9_AUTHORED_SHEETS_PREVIEW_ONLY',
              'frame_order': 'row-major, left-to-right then top-to-bottom',
              'sprite_origin': [32, 6], 'sprite_size': [64, 44],
              'extra_heart_or_idle_motion': False, 'roles': []}
    font = ImageFont.truetype('C:/Windows/Fonts/msjh.ttc', 18)
    overview = Image.new('RGB', (1056, 4 * 180), '#171b21')
    draw = ImageDraw.Draw(overview)
    for row, (key, label, fps, count) in enumerate(ROLES):
        species, stage = key.split('-')
        source = ROOT / f'assets/pets/{species}/{stage}/happy.png'
        art = Image.open(source).convert('RGBA')
        assert art.width % 64 == 0 and art.height % 44 == 0
        assert all(a in (0, 255) and (not a or (r, g, b) == (255, 255, 255))
                   for r, g, b, a in art.get_flattened_data())
        tiles = [art.crop((x, y, x + 64, y + 44)).getchannel('A')
                 for y in range(0, art.height, 44) for x in range(0, art.width, 64)]
        assert len(tiles) >= count
        assert all(tile.getbbox() for tile in tiles[:count])
        assert all(tile.getbbox() is None for tile in tiles[count:])
        template = Image.open(BASE / f'{key}-idle-0-128x64.png').convert('L')
        frames = []
        frame_dir = OUT / key
        frame_dir.mkdir(exist_ok=True)
        for index, tile in enumerate(tiles[:count]):
            screen = template.copy()
            screen.paste(tile, (32, 6))
            assert screen.crop((32, 6, 96, 50)).tobytes() == tile.tobytes()
            # Every pixel outside the complete user-authored tile stays unchanged.
            outside = screen.copy()
            outside.paste(template.crop((32, 6, 96, 50)), (32, 6))
            assert outside.tobytes() == template.tobytes()
            screen.save(frame_dir / f'frame-{index + 1:02d}-128x64.png')
            frames.append(screen)

        # GIF delays use 10ms units. 7 repetitions of an 8-frame/7-FPS
        # sequence yield exactly 56 frames / 8 seconds, with cumulative
        # boundary rounding error below 5ms and no long-term speed drift.
        repeats = 1 if fps == 5 else 7
        timeline = frames * repeats
        boundaries = [(i * 100 + fps // 2) // fps * 10
                      for i in range(len(timeline) + 1)]
        durations = [b - a for a, b in zip(boundaries, boundaries[1:])]
        assert all(d > 0 and d % 10 == 0 for d in durations)
        assert len(timeline) * 1000 == sum(durations) * fps
        assert max(abs(t - i * 1000 / fps) for i, t in enumerate(boundaries)) < 5
        outputs = []
        for scale in (1, 4):
            images = [f.resize((128 * scale, 64 * scale), Image.Resampling.NEAREST)
                      for f in timeline]
            outputs.append(save_verified(OUT / f'{key}-happy-{128*scale}x{64*scale}.gif',
                                         images, durations))
        draw.text((12, row * 180 + 6), f'{label}｜{fps} FPS｜{count} 幀｜完全依照新圖集',
                  font=font, fill='white')
        for column, index in enumerate([0, count // 3, count * 2 // 3, count - 1]):
            overview.paste(frames[index].resize((256, 128), Image.Resampling.NEAREST),
                           (8 + column * 264, row * 180 + 40))
        report['roles'].append({'role': key, 'fps': fps, 'source': source.relative_to(ROOT).as_posix(),
                                'source_sha256': digest(source), 'sheet_size': list(art.size),
                                'authored_frames': count, 'trailing_empty_cells_skipped': len(tiles) - count,
                                'source_frame_order_1_based': list(range(1, count + 1)),
                                'gif_artwork_repetitions': repeats, 'frame_durations_ms': durations,
                                'cycle_ideal_ms': count * 1000 / fps,
                                'maximum_boundary_rounding_error_ms': max(abs(t-i*1000/fps) for i,t in enumerate(boundaries)),
                                'hud_and_sprite_pixels_verified': True, 'gifs': outputs})
    overview.save(OUT / 'overview.png')
    assert all(digest(ROOT / name) == value for name, value in before.items())
    report['sources_firmware_and_earlier_previews_unchanged'] = True
    report['protected_file_sha256'] = before
    (OUT / 'validation.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (OUT / 'README.md').write_text('''# 摸頭 V9：使用者重繪動作與愛心時序

直接依 assets/pets 各角色 happy.png 的 64×44 格子，由左到右、由上到下播放。
幼鳥 14 幀、成鳥 13 幀，各 5 FPS（每幀 200ms）；略過圖集末尾的 1／2 格透明填充。
幼龍、成龍各 8 幀，7 FPS。角色與愛心都直接使用使用者整幀原像素，
不再加入 V8 的愛心漂移、生成時差或其他待機疊加，也不調整愛心座標。

原點保留 (32,6)，OLED 128×64；沿用先前真實 C++／U8g2 Home framebuffer 的 HUD。
另提供最近鄰放大的 512×256 GIF。每個角色子資料夾含全部原尺寸 PNG。

GIF 延遲以 10ms 為單位，無法逐格精確表示 1/7 秒。
小飛龍採 140／150ms 累積捨入，每個時間邊界誤差小於 5ms；
GIF 連續保留 7 輪共 56 格／8 秒，平均正好 7 FPS，畫面順序不變。
小鳥 GIF 各一輪：幼鳥 2.8 秒、成鳥 2.6 秒，均連續循環。

已逐格回讀 8 份 GIF，核對像素、順序、時長、HUD 保留與放大一致性；
原始素材、正式韌體及既有 V4～V8 預覽雜湊未變。
這次僅製作預覽，未修改正式韌體、未進行 Build／觸摸或 OLED 實機驗收。

重建：使用有 Pillow 的 Python 執行 tools/render_touch_authored_timing_preview.py。
''', encoding='utf-8')
    for role in report['roles']:
        print(role['role'], role['authored_frames'], 'frames', role['fps'], 'FPS', role['gifs'])
    print('Sources, firmware, older previews, all frame pixels and durations verified.')


if __name__ == '__main__':
    main()
