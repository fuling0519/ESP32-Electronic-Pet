@echo off
setlocal
cd /d "%~dp0"
set "DIRTY_PIO=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
if exist "%DIRTY_PIO%" (
    "%DIRTY_PIO%" run -e esp32dev_dirty_preview -t upload
) else (
    pio run -e esp32dev_dirty_preview -t upload
)
if errorlevel 1 (
    echo Upload failed. Check USB connection and close any serial monitor.
) else (
    echo Preview uploaded. Cycles every 4 seconds.
    echo Left/Right: stage. Press: pause/resume. Up: eat. Down: sleep/wake.
)
pause
