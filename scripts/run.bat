@echo off
REM ============================================================
REM  SynGT C++ - Run Script for Windows
REM ============================================================

set PROJECT_ROOT=%~dp0..
set EXE_PATH=%PROJECT_ROOT%\build\bin\syngt_cli.exe

if not exist "%EXE_PATH%" (
    echo Executable not found: %EXE_PATH%
    echo Please build the project first: scripts\build.bat
    exit /b 1
)

REM
if "%~1"=="" (
    "%EXE_PATH%" --help
) else (
    "%EXE_PATH%" %*
)