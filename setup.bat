@echo off
echo ========================================
echo   SynGT GUI Setup Script
echo ========================================
echo.

cd /d "%~dp0"

REM Проверяем наличие git
where git >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Git not found! Please install Git first.
    pause
    exit /b 1
)

REM Создаем директорию syngt_gui если нет
if not exist "syngt_gui" mkdir syngt_gui
cd syngt_gui

REM Скачиваем ImGui если нет
if exist "imgui" (
    echo ImGui already exists, updating...
    cd imgui
    git pull
    cd ..
) else (
    echo Downloading Dear ImGui...
    git clone --depth 1 https://github.com/ocornut/imgui.git
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Failed to clone ImGui repository
        pause
        exit /b 1
    )
)

echo.
echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Copy syngt_gui_main.cpp to syngt_gui/main.cpp
echo 2. Run: cmake -B build -G "MinGW Makefiles" -DBUILD_GUI=ON
echo 3. Run: cmake --build build
echo.
pause