@echo off

setlocal

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

set PROTOTYPE=07-ipc

if not exist build\%PROTOTYPE% (
    mkdir build\%PROTOTYPE%
)

cl ^
/EHsc ^
/std:c++17 ^
/DUNICODE ^
/D_UNICODE ^
prototypes\%PROTOTYPE%\main.cpp ^
/I external\webview2\include ^
/link ^
/LIBPATH:external\webview2\x64 ^
WebView2LoaderStatic.lib ^
dcomp.lib ^
user32.lib ^
gdi32.lib ^
ole32.lib ^
shell32.lib ^
windowscodecs.lib ^
advapi32.lib ^
wevtapi.lib ^
/OUT:build\%PROTOTYPE%\prototype.exe

if errorlevel 1 (
    echo.
    echo ===== BUILD FAILED =====
    exit /b 1
)

echo.
echo ===== BUILD SUCCESS =====
echo.
echo Launching...

start "" build\%PROTOTYPE%\prototype.exe