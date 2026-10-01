# Run from the repository root. Requires native gcc/g++ and an existing pio build.
$ErrorActionPreference = 'Stop'
$outputPath = Join-Path (Get-Location) '.pio/rps-preview'
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
$fontSource = '.pio/libdeps/esp32dev/U8g2/src/clib'
if (-not (Test-Path "$fontSource/u8g2.h")) { throw 'Build esp32dev first to install U8g2.' }

& g++ -std=c++11 -Wall -Wextra -Isrc test/rps_game_native.cpp src/games/RpsGame.cpp -o "$outputPath/rps-game.exe"
if ($LASTEXITCODE) { throw 'Game test build failed.' }
& "$outputPath/rps-game.exe"
if ($LASTEXITCODE) { throw 'Game tests failed.' }

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
    'src/games/RpsGame.cpp','src/pet/PetData.cpp','src/pet/PetSnapshot.cpp','src/storage/Memorials.cpp')
& g++ -std=c++11 -Wall -Wextra -ffunction-sections -fdata-sections '-Wl,--gc-sections' `
    -Itest/support -Isrc -Iinclude "-I$fontSource" @sources @objects -o "$outputPath/rps-ui.exe"
if ($LASTEXITCODE) { throw 'UI test build failed.' }
& "$outputPath/rps-ui.exe"
if ($LASTEXITCODE) { throw 'UI tests failed.' }

& g++ -std=c++11 -Wall -Wextra -Isrc test/pet_core_native.cpp src/pet/PetData.cpp `
    src/pet/PetSnapshot.cpp src/pet/PetClock.cpp src/pet/PetName.cpp src/storage/Memorials.cpp `
    -o "$outputPath/pet-core.exe"
if ($LASTEXITCODE) { throw 'Pet test build failed.' }
& "$outputPath/pet-core.exe"
if ($LASTEXITCODE) { throw 'Pet regression tests failed.' }
Write-Output 'PASS: pet core regression suite.'
