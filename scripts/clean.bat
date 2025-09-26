@echo off
echo Cleaning build directory...
if exist "%~dp0..\build" (
    rmdir /s /q "%~dp0..\build"
    echo Cleaned successfully
) else (
    echo Build directory not found
)