<#
.SYNOPSIS
    Switch PlatformIO environment for Wokwi simulation.

.DESCRIPTION
    Builds the specified PlatformIO environment and updates wokwi.toml to
    point to the resulting firmware. Then you can launch the Wokwi simulator
    from VS Code (Ctrl+Shift+P -> "Wokwi: Start Simulator").

.PARAMETER Env
    PlatformIO environment name (e.g. esp32dev, esp32dev_sad_test).
    Defaults to 'esp32dev'.

.PARAMETER ListOnly
    List all available environments without building.

.PARAMETER NoBuild
    Update wokwi.toml without rebuilding (assumes firmware already exists).

.EXAMPLE
    .\tools\wokwi-env.ps1                           # Build & switch to esp32dev
    .\tools\wokwi-env.ps1 -Env esp32dev_sad_test     # Build & switch to sad test
    .\tools\wokwi-env.ps1 -ListOnly                  # List all environments
    .\tools\wokwi-env.ps1 -Env esp32dev_death_test -NoBuild  # Switch without rebuild
#>
param(
    [string]$Env = "esp32dev",
    [switch]$ListOnly,
    [switch]$NoBuild
)

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$iniPath = Join-Path $projectRoot "platformio.ini"
$wokwiPath = Join-Path $projectRoot "wokwi.toml"
$pioCommand = Get-Command pio -ErrorAction SilentlyContinue
$pioPath = if ($pioCommand) {
    $pioCommand.Source
} else {
    Join-Path $HOME ".platformio\penv\Scripts\pio.exe"
}

# Parse all environments from platformio.ini
$envNames = @()
Get-Content $iniPath | ForEach-Object {
    if ($_ -match '^\[env:(.+)\]') {
        $envNames += $Matches[1]
    }
}

# Environment descriptions (for display)
$envDescriptions = @{
    "esp32dev"                            = "正式版（預設）"
    "esp32dev_death_test"                 = "死亡測試"
    "esp32dev_touch_read_test"            = "觸摸原始讀值測試"
    "esp32dev_memorial_sample"            = "紀念冊範例"
    "esp32dev_growth_test"                = "成長測試"
    "esp32dev_memorial_test"              = "紀念冊測試（含成長）"
    "esp32dev_sleep_test"                 = "一般睡眠測試"
    "esp32dev_dirty_preview"              = "髒污預覽"
    "esp32dev_deep_sleep_test"            = "深度睡眠測試"
    "esp32dev_treatment_test"             = "治療測試（成年）"
    "esp32dev_treatment_baby_test"        = "治療測試（幼年）"
    "esp32dev_sad_test"                   = "傷心表情測試（成年）"
    "esp32dev_sad_baby_test"              = "傷心表情測試（幼年）"
    "esp32dev_wyvern_test"                = "小飛龍測試（成年）"
    "esp32dev_wyvern_baby_test"           = "小飛龍測試（幼年）"
    "esp32dev_wyvern_treatment_test"      = "小飛龍治療測試（成年）"
    "esp32dev_wyvern_treatment_baby_test" = "小飛龍治療測試（幼年）"
    "esp32dev_nvs_maintenance"            = "NVS 維護工具"
}

if ($ListOnly) {
    Write-Host ""
    Write-Host "Available PlatformIO environments for Wokwi simulation:" -ForegroundColor Cyan
    Write-Host "========================================================" -ForegroundColor Cyan
    Write-Host ""
    foreach ($name in $envNames) {
        $desc = if ($envDescriptions.ContainsKey($name)) { $envDescriptions[$name] } else { "" }
        $marker = ""
        # Check if firmware exists
        $fwPath = Join-Path $projectRoot ".pio\build\$name\firmware.bin"
        if (Test-Path $fwPath) { $marker = " [built]" }
        Write-Host ("  {0,-42} {1}{2}" -f $name, $desc, $marker)
    }
    Write-Host ""
    Write-Host "Usage: .\tools\wokwi-env.ps1 -Env <environment_name>" -ForegroundColor Yellow
    exit 0
}

# Validate environment
if ($envNames -notcontains $Env) {
    Write-Host "ERROR: Unknown environment '$Env'" -ForegroundColor Red
    Write-Host "Available: $($envNames -join ', ')" -ForegroundColor Yellow
    exit 1
}

$desc = if ($envDescriptions.ContainsKey($Env)) { " ($($envDescriptions[$Env]))" } else { "" }
Write-Host ""
Write-Host "Wokwi Environment Switcher" -ForegroundColor Cyan
Write-Host "==========================" -ForegroundColor Cyan
Write-Host "Target: $Env$desc" -ForegroundColor Green

# Build if needed
$firmwareBin = ".pio/build/$Env/firmware.bin"
$firmwareElf = ".pio/build/$Env/firmware.elf"
$fullBinPath = Join-Path $projectRoot $firmwareBin

if (-not $NoBuild) {
    if (-not (Test-Path $pioPath)) {
        Write-Host "ERROR: PlatformIO CLI not found. Add 'pio' to PATH or install it under $HOME\.platformio." -ForegroundColor Red
        exit 1
    }
    Write-Host ""
    Write-Host "Building $Env..." -ForegroundColor Yellow
    Push-Location $projectRoot
    & $pioPath run -e $Env
    $buildResult = $LASTEXITCODE
    Pop-Location
    if ($buildResult -ne 0) {
        Write-Host "BUILD FAILED (exit code $buildResult)" -ForegroundColor Red
        exit $buildResult
    }
    Write-Host "Build succeeded." -ForegroundColor Green
} else {
    if (-not (Test-Path $fullBinPath)) {
        Write-Host "ERROR: Firmware not found at $firmwareBin" -ForegroundColor Red
        Write-Host "Run without -NoBuild to build first." -ForegroundColor Yellow
        exit 1
    }
    Write-Host "Skipping build (using existing firmware)." -ForegroundColor Yellow
}

# Update wokwi.toml
$wokwiContent = @"
[wokwi]
version = 1
firmware = "$firmwareBin"
elf = "$firmwareElf"
"@

Set-Content -Path $wokwiPath -Value $wokwiContent -NoNewline
Write-Host ""
Write-Host "Updated wokwi.toml -> $Env" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. In VS Code: Ctrl+Shift+P -> 'Wokwi: Start Simulator'" -ForegroundColor White
Write-Host "  2. Or restart the simulator if already running" -ForegroundColor White
Write-Host ""
