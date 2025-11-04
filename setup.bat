@echo off
echo ========================================
echo   SynGT GUI Setup Script
echo ========================================
echo.

cd /d "%~dp0"

where git >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Git not found! Please install Git first.
    pause
    exit /b 1
)

if not exist "syngt_gui" mkdir syngt_gui
cd syngt_gui

echo Downloading Dear ImGui...
git clone --depth 1 https://github.com/ocornut/imgui.git
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to clone ImGui repository
    pause
    exit /b 1
)
cd ..

echo.
echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Run: cmake -B build -G Ninja -DBUILD_GUI=ON
echo 2. Run: cd build 
echo 3. Run: ninja
echo.
echo Wait for the compilation  and than project will be in build/bin/syngt_gui.exe
pause