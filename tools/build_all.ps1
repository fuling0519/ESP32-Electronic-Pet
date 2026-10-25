# Builds PlatformIO environments several at a time and prints one summary table.
#
# Day to day: build only the production firmware (pio run -e esp32dev) and run tools/test_rps.ps1.
# Build every environment before a commit only when the change can compile differently under the test
# environments' flags. -Auto decides that from git: platformio.ini, any header under src/ or include/, or any
# source that uses a PET_*_TEST/SAMPLE/PREVIEW/MAINTENANCE flag means all environments; otherwise esp32dev only.
#
#   tools/build_all.ps1              # every environment, 4 at a time
#   tools/build_all.ps1 -Auto        # every environment only if the working tree needs it
#   tools/build_all.ps1 -Jobs 6 -Environments esp32dev,esp32dev_sad_test
param(
    [int]$Jobs = 4,
    [string[]]$Environments,
    [switch]$Auto
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe'
if (-not (Test-Path $pio)) { $pio = 'pio' }

$all = Select-String -Path (Join-Path $root 'platformio.ini') -Pattern '^\[env:([^\]]+)\]' |
    ForEach-Object { $_.Matches[0].Groups[1].Value }
$targets = if ($Environments) { $Environments } else { $all }

if ($Auto -and -not $Environments) {
    # Changed against HEAD, staged or not, plus new untracked files.
    $changed = @(git -C $root diff --name-only HEAD) + @(git -C $root ls-files --others --exclude-standard)
    $flagged = @(git -C $root grep -lE 'PET_[A-Z_]*(TEST|SAMPLE|PREVIEW|MAINTENANCE)' -- src include)
    $needsAll = $changed | Where-Object {
        $_ -eq 'platformio.ini' -or
        ($_ -match '^(src|include)/' -and ($_ -match '\.h$' -or $flagged -contains $_))
    }
    if ($needsAll) {
        Write-Output ("Flag-sensitive changes, building all: " + (($needsAll | Select-Object -Unique) -join ', '))
    } else {
        Write-Output 'No flag-sensitive firmware changes: building esp32dev only.'
        $targets = @('esp32dev')
    }
}

$logDir = Join-Path $root '.pio\build-all-logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$queue = New-Object System.Collections.Queue
foreach ($name in $targets) { $queue.Enqueue($name) }
$running = @{}
$results = @()
$startedAll = Get-Date

while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($queue.Count -gt 0 -and $running.Count -lt $Jobs) {
        $name = $queue.Dequeue()
        $log = Join-Path $logDir "$name.log"
        $process = Start-Process -FilePath $pio -ArgumentList @('run', '-e', $name) -WorkingDirectory $root `
            -RedirectStandardOutput $log -RedirectStandardError "$log.err" -NoNewWindow -PassThru
        $null = $process.Handle  # keep the handle so ExitCode is still readable after the process ends
        $running[$name] = @{ Process = $process; Started = Get-Date; Log = $log }
    }
    Start-Sleep -Milliseconds 500
    foreach ($name in @($running.Keys)) {
        $entry = $running[$name]
        if (-not $entry.Process.HasExited) { continue }
        $flash = Select-String -Path $entry.Log -Pattern 'Flash:.*\(used (\d+) bytes' |
            Select-Object -Last 1 | ForEach-Object { $_.Matches[0].Groups[1].Value }
        $results += [pscustomobject]@{
            Environment = $name
            Status = if ($entry.Process.ExitCode -eq 0) { 'SUCCESS' } else { 'FAILED' }
            Seconds = [int]((Get-Date) - $entry.Started).TotalSeconds
            FlashBytes = $flash
        }
        $running.Remove($name)
    }
}

$results | Sort-Object Environment | Format-Table -AutoSize | Out-String -Width 160 | Write-Output
$failed = @($results | Where-Object Status -ne 'SUCCESS')
Write-Output ("{0} environment(s) in {1:N0} s, {2} at a time; logs in .pio/build-all-logs." -f `
    $results.Count, ((Get-Date) - $startedAll).TotalSeconds, $Jobs)
if ($failed.Count) {
    Write-Output ("FAILED: " + (($failed | ForEach-Object Environment) -join ', '))
    exit 1
}
