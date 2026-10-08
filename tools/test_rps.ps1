# Run from the repository root. Requires native gcc/g++ and an existing pio build.
$ErrorActionPreference = 'Stop'
$outputPath = Join-Path (Get-Location) '.pio/rps-preview'
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
& g++ -std=c++11 -Wall -Wextra -Itest/support -Isrc test/device_settings_native.cpp src/storage/DeviceSettings.cpp -o "$outputPath/device-settings.exe"
if ($LASTEXITCODE) { throw 'Device settings test build failed.' }
& "$outputPath/device-settings.exe"
if ($LASTEXITCODE) { throw 'Device settings tests failed.' }
& g++ -std=c++11 -Wall -Wextra -DPET_DEEP_SLEEP_TEST_MODE=1 -Itest/support -Isrc test/device_settings_native.cpp src/storage/DeviceSettings.cpp -o "$outputPath/device-settings-test.exe"
if ($LASTEXITCODE) { throw 'Test settings namespace build failed.' }
& "$outputPath/device-settings-test.exe"
if ($LASTEXITCODE) { throw 'Test settings namespace tests failed.' }
& g++ -std=c++11 -Wall -Wextra -Itest/support -Isrc -Iinclude test/sound_native.cpp src/hardware/Sound.cpp -o "$outputPath/sound.exe"
if ($LASTEXITCODE) { throw 'Sound test build failed.' }
& "$outputPath/sound.exe"
if ($LASTEXITCODE) { throw 'Sound tests failed.' }
& g++ -std=c++11 -Wall -Wextra -DPET_SAD_TEST_MODE=1 -Isrc test/sad_quick_native.cpp src/pet/PetData.cpp src/pet/PetSnapshot.cpp -o "$outputPath/sad-quick.exe"
if ($LASTEXITCODE) { throw 'Sad quick test build failed.' }
& "$outputPath/sad-quick.exe"
if ($LASTEXITCODE) { throw 'Sad quick tests failed.' }
$fontSource = '.pio/libdeps/esp32dev/U8g2/src/clib'
if (-not (Test-Path "$fontSource/u8g2.h")) { throw 'Build esp32dev first to install U8g2.' }

& g++ -std=c++11 -Wall -Wextra -Isrc test/rps_game_native.cpp src/games/RpsGame.cpp -o "$outputPath/rps-game.exe"
if ($LASTEXITCODE) { throw 'Game test build failed.' }
& "$outputPath/rps-game.exe"
if ($LASTEXITCODE) { throw 'Game tests failed.' }

& g++ -std=c++11 -Wall -Wextra -Isrc test/memory_notes_native.cpp src/games/MemoryNotes.cpp -o "$outputPath/memory-notes.exe"
if ($LASTEXITCODE) { throw 'Memory notes test build failed.' }
& "$outputPath/memory-notes.exe"
if ($LASTEXITCODE) { throw 'Memory notes tests failed.' }

$units = @('u8g2_box','u8g2_circle','u8g2_font','u8g2_fonts','u8g2_hvline',
    'u8g2_intersection','u8g2_kerning','u8g2_line','u8g2_ll_hvline','u8g2_setup',
    'u8x8_8x8','u8x8_byte','u8x8_cad','u8x8_display','u8x8_gpio','u8x8_setup')
$objects = @()
foreach ($unit in $units) {
    $object = "$outputPath/$unit.o"
    & gcc -ffunction-sections -fdata-sections "-I$fontSource" -c "$fontSource/$unit.c" -o $object
    if ($LASTEXITCODE) { throw "Font build failed: $unit" }
    $objects += $object
}
$sources = @('test/rps_ui_native.cpp','src/ui/UiController.cpp','src/ui/PetIcons.cpp',
    'src/games/RpsGame.cpp','src/games/MemoryNotes.cpp','src/pet/PetData.cpp','src/pet/PetSnapshot.cpp','src/storage/Memorials.cpp')
& g++ -std=c++11 -Wall -Wextra -ffunction-sections -fdata-sections '-Wl,--gc-sections' `
    -Itest/support -Isrc -Iinclude "-I$fontSource" @sources @objects -o "$outputPath/rps-ui.exe"
if ($LASTEXITCODE) { throw 'UI test build failed.' }
& "$outputPath/rps-ui.exe"
if ($LASTEXITCODE) { throw 'UI tests failed.' }
& g++ -std=c++11 -Wall -Wextra -DPET_SAD_TEST_MODE=1 -ffunction-sections -fdata-sections '-Wl,--gc-sections' `
    -Itest/support -Isrc -Iinclude "-I$fontSource" @sources @objects -o "$outputPath/sad-ui.exe"
if ($LASTEXITCODE) { throw 'Sad quick UI test build failed.' }
& "$outputPath/sad-ui.exe"
if ($LASTEXITCODE) { throw 'Sad quick UI tests failed.' }

& g++ -std=c++11 -Wall -Wextra -Isrc test/pet_core_native.cpp src/pet/PetData.cpp `
    src/pet/PetSnapshot.cpp src/pet/PetClock.cpp src/pet/PetName.cpp src/storage/Memorials.cpp `
    -o "$outputPath/pet-core.exe"
if ($LASTEXITCODE) { throw 'Pet test build failed.' }
& "$outputPath/pet-core.exe"
if ($LASTEXITCODE) { throw 'Pet regression tests failed.' }
Write-Output 'PASS: pet core regression suite.'

& g++ -std=c++11 -Wall -Wextra -Isrc test/mood_native.cpp src/pet/PetData.cpp `
    src/pet/PetSnapshot.cpp -o "$outputPath/mood.exe"
if ($LASTEXITCODE) { throw 'Mood test build failed.' }
& "$outputPath/mood.exe"
if ($LASTEXITCODE) { throw 'Mood tests failed.' }
Write-Output 'PASS: mood thresholds, recovery progress and batch settlement.'

& g++ -std=c++11 -Wall -Wextra -Itest/support -Isrc test/farewell_native.cpp `
    src/pet/PetData.cpp src/pet/PetSnapshot.cpp src/storage/Memorials.cpp `
    -o "$outputPath/farewell.exe"
if ($LASTEXITCODE) { throw 'Farewell storage test build failed.' }
& "$outputPath/farewell.exe"
if ($LASTEXITCODE) { throw 'Farewell storage tests failed.' }
