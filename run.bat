@echo off

if "%~1"=="" (
    echo Usage:
    echo run 07-ipc
    exit /b
)

build\%1\prototype.exe