# Run after tools/test_rps.ps1 has built the real U8g2 host objects.
$ErrorActionPreference = 'Stop'
$touchOutput = Join-Path (Get-Location) '.pio/rps-preview'
New-Item -ItemType Directory -Force -Path $touchOutput | Out-Null
& g++ -std=c++11 -Wall -Wextra -Itest/support -Isrc -Iinclude test/touch_native.cpp src/hardware/TouchGesture.cpp src/hardware/TouchInput.cpp -o "$touchOutput/touch.exe"
if ($LASTEXITCODE) { throw 'Touch input test build failed.' }
& "$touchOutput/touch.exe"
if ($LASTEXITCODE) { throw 'Touch input tests failed.' }
$touchFontSource = '.pio/libdeps/esp32dev/U8g2/src/clib'
$touchObjects = @(Get-ChildItem -LiteralPath $touchOutput -Filter 'u8*.o' | ForEach-Object { $_.FullName })
if (-not $touchObjects.Count) { throw 'Run tools/test_rps.ps1 first to build U8g2 objects.' }
$touchSources = @('test/touch_ui_native.cpp','src/ui/UiController.cpp','src/ui/PetIcons.cpp',
    'src/games/RpsGame.cpp','src/games/MemoryNotes.cpp','src/pet/PetData.cpp',
    'src/pet/PetSnapshot.cpp','src/pet/PetClock.cpp','src/storage/Memorials.cpp')
& g++ -std=c++11 -Wall -Wextra -ffunction-sections -fdata-sections '-Wl,--gc-sections' `
    -Itest/support -Isrc -Iinclude "-I$touchFontSource" @touchSources @touchObjects -o "$touchOutput/touch-ui.exe"
if ($LASTEXITCODE) { throw 'Touch UI test build failed.' }
& "$touchOutput/touch-ui.exe"
if ($LASTEXITCODE) { throw 'Touch UI tests failed.' }
