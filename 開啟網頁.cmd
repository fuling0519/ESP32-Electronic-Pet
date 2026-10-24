@echo off
rem Double-click to open the web preview at http://localhost:8765/.
rem Opening index.html directly (file://) blocks canvas pixel reads, which breaks the
rem death halo, light motes, farewell photos and the touch hit area.
setlocal
set PORT=8765
set URL=http://localhost:%PORT%/

rem Already running (an earlier double-click or the dev preview): just open the page.
curl.exe -s -o nul %URL% && goto open

set PY=
where python >nul 2>nul && set PY=python
if not defined PY where py >nul 2>nul && set PY=py
if not defined PY goto nopython

start "ESP32 Pet web server (close this window to stop)" /min %PY% -m http.server %PORT% --bind 127.0.0.1 --directory "%~dp0web-preview"
for /l %%i in (1,1,20) do (
  curl.exe -s -o nul %URL% && goto open
  timeout /t 1 /nobreak >nul
)
echo The web server did not start on port %PORT%. Check the minimized server window for errors.
pause
exit /b 1

:open
start "" %URL%
exit /b 0

:nopython
echo Python was not found. Install Python 3, or open web-preview\index.html directly
echo (the death halo, light motes and farewell photos will not work that way).
pause
exit /b 1
