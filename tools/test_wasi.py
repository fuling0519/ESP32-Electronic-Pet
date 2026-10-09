"""Run host regressions in Node WASI when Windows blocks generated executables.

Uses the existing portable Zig toolchain. No firmware or system settings change.
"""
from pathlib import Path
import argparse
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--node', required=True, help='Existing Node executable with WASI support')
args = parser.parse_args()
zig = ROOT / '.pio/host-tools/zig-windows-x86_64-0.13.0/zig.exe'
out = ROOT / '.pio/wasi-tests'
out.mkdir(parents=True, exist_ok=True)
os.chdir(ROOT)
os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(ROOT / '.pio/zig-cache')
font = ROOT / '.pio/libdeps/esp32dev/U8g2/src/clib'
cache = {}

def run(command):
    subprocess.run([str(x) for x in command], check=True)

def compile_unit(source, flags=(), library=False):
    key = (str(source), tuple(flags), library)
    if key in cache: return cache[key]
    obj = out / f'unit-{len(cache)}.o'
    cpp = str(source).endswith('.cpp')
    command = [zig, 'c++' if cpp else 'cc', '-target', 'wasm32-wasi',
               '-ffunction-sections', '-fdata-sections', '-Itest/support', '-Isrc',
               '-Iinclude', '-I'+str(font)]
    if cpp: command += ['-std=c++20' if library else '-std=c++11', '-fno-exceptions', '-fno-rtti']
    if library: command += ['-D_LIBCPP_BUILDING_LIBRARY']
    run(command + list(flags) + ['-c', source, '-o', obj])
    cache[key] = obj
    return obj

# Only needed single-threaded libc++ components, rather than Zig's full WASI
# libc++ build (which attempts unsupported thread implementations in 0.13).
runtime = [compile_unit(zig.parent / 'lib/libcxx/src' / name, library=True)
           for name in ('new.cpp', 'new_handler.cpp', 'new_helpers.cpp', 'verbose_abort.cpp', 'string.cpp')]
pet = ['src/pet/PetData.cpp', 'src/pet/PetSnapshot.cpp']
units = ['u8g2_box','u8g2_circle','u8g2_font','u8g2_fonts','u8g2_hvline',
         'u8g2_intersection','u8g2_kerning','u8g2_line','u8g2_ll_hvline','u8g2_setup',
         'u8x8_8x8','u8x8_byte','u8x8_cad','u8x8_display','u8x8_gpio','u8x8_setup']
ui = ['test/rps_ui_native.cpp','src/ui/UiController.cpp','src/ui/PetIcons.cpp',
      'src/games/RpsGame.cpp','src/games/MemoryNotes.cpp'] + pet + ['src/storage/Memorials.cpp']
targets = [
 ('device-settings', ['test/device_settings_native.cpp','src/storage/DeviceSettings.cpp'], []),
 ('device-settings-test', ['test/device_settings_native.cpp','src/storage/DeviceSettings.cpp'], ['-DPET_DEEP_SLEEP_TEST_MODE=1']),
 ('sound', ['test/sound_native.cpp','src/hardware/Sound.cpp'], []),
 ('rps-game', ['test/rps_game_native.cpp','src/games/RpsGame.cpp'], []),
 ('memory-notes', ['test/memory_notes_native.cpp','src/games/MemoryNotes.cpp'], []),
 ('pet-core', ['test/pet_core_native.cpp']+pet+['src/pet/PetClock.cpp','src/pet/PetName.cpp','src/storage/Memorials.cpp'], []),
 ('mood', ['test/mood_native.cpp']+pet, []),
 ('sad-quick', ['test/sad_quick_native.cpp']+pet, ['-DPET_SAD_TEST_MODE=1']),
 ('farewell', ['test/farewell_native.cpp']+pet+['src/storage/Memorials.cpp'], []),
 ('wyvern', ['test/wyvern_native.cpp']+pet+['src/storage/Memorials.cpp'], []),
 ('storage-retry', ['test/storage_retry_native.cpp']+pet+['src/storage/Memorials.cpp'], []),
 ('rps-ui', ui, []),
 ('sad-ui', ui, ['-DPET_SAD_TEST_MODE=1']),
]
for name, sources, flags in targets:
    objects = [compile_unit(source, flags) for source in sources]
    if name in ('rps-ui','sad-ui'):
        objects += [compile_unit(font/(unit+'.c')) for unit in units]
    module = out / (name+'.wasm')
    run([zig,'cc','-target','wasm32-wasi']+objects+runtime+['-o',module])
    run([args.node,'--no-warnings',ROOT/'tools/run_wasi_test.cjs',module])
    print('PASS WASI:',name,flush=True)
