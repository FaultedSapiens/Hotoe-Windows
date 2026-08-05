@echo off

if "%~1"=="" (
    echo Usage:
    echo dev 7 ipc
    exit /b
)

call build.bat %1

if errorlevel 1 exit /b

call run.bat %1