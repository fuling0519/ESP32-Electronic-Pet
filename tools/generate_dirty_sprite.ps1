Add-Type -AssemblyName System.Drawing
$root = Split-Path $PSScriptRoot -Parent
$bitmap = [System.Drawing.Bitmap]::new((Join-Path $root 'assets/dirty/dirty.png'))
try {
    if ($bitmap.Width -ne 38 -or $bitmap.Height -ne 48) { throw 'Expected a 38x48 sheet of 19x24 tiles.' }
    $lines = @('#pragma once', '#include <stdint.h>', '// Generated from assets/dirty/dirty.png. Row-major, MSB-first; zero = white.', 'namespace Ui { namespace PetIcons {', 'constexpr uint8_t kDirtWidth = 19;', 'constexpr uint8_t kDirtHeight = 24;', 'const uint8_t kDirtFrames[3][72] PROGMEM = {')
    for ($frame = 0; $frame -lt 3; $frame++) {
        $bytes = @()
        for ($y = 0; $y -lt 24; $y++) {
            for ($column = 0; $column -lt 3; $column++) {
                $value = 255
                for ($bit = 0; $bit -lt 8; $bit++) {
                    $x = $column * 8 + $bit
                    if ($x -ge 19) { continue }
                    $pixel = $bitmap.GetPixel(($frame % 2) * 19 + $x, [int][Math]::Floor($frame / 2) * 24 + $y)
                    if ($pixel.A -gt 127) {
                        if ($pixel.R -ne 255 -or $pixel.G -ne 255 -or $pixel.B -ne 255) { throw 'Expected white or transparent pixels.' }
                        $value = $value -band (255 -bxor (128 -shr $bit))
                    }
                }
                $bytes += ('0x{0:X2}' -f $value)
            }
        }
        $lines += '    {' + ($bytes -join ', ') + '},'
    }
    $lines += '};', '} } // namespace Ui::PetIcons'
    $lines | Set-Content -Encoding ascii (Join-Path $root 'src/ui/DirtySprite.h')
} finally { $bitmap.Dispose() }
