@echo off
setlocal
where flutter >nul 2>nul
if errorlevel 1 (
  echo Flutter SDK is not installed or is missing from PATH.
  echo Install Flutter for Windows, then reopen this terminal.
  exit /b 1
)
set SCAFFOLD=%TEMP%\esp32_pet_scaffold_%RANDOM%
flutter create --platforms=windows,android,ios,web --project-name esp32_pet_app "%SCAFFOLD%"
if errorlevel 1 exit /b 1
xcopy /E /I /Y "%SCAFFOLD%\windows" "windows" >nul
xcopy /E /I /Y "%SCAFFOLD%\android" "android" >nul
xcopy /E /I /Y "%SCAFFOLD%\ios" "ios" >nul
xcopy /E /I /Y "%SCAFFOLD%\web" "web" >nul
copy /Y "%SCAFFOLD%\.metadata" ".metadata" >nul
copy /Y "%SCAFFOLD%\.gitignore" ".gitignore" >nul
rmdir /S /Q "%SCAFFOLD%"
flutter pub get
if errorlevel 1 exit /b 1
echo.
echo Setup complete. Run run_windows.bat to launch the Windows app.
