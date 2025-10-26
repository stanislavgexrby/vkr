@echo off
echo ========================================
echo   SynGT GUI - Build and Run
echo ========================================
echo.

REM Переходим в корень проекта
cd /d "%~dp0"

REM Проверяем наличие ImGui
if not exist "syngt_gui\imgui\imgui.h" (
    echo ERROR: ImGui not found!
    echo Please run setup_imgui.bat first.
    echo.
    pause
    exit /b 1
)

REM Конфигурация CMake (если нет build директории)
if not exist "build" (
    echo Configuring CMake...
    cmake -B build -G "MinGW Makefiles" -DBUILD_GUI=ON -DCMAKE_BUILD_TYPE=Release
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: CMake configuration failed!
        pause
        exit /b 1
    )
)

REM Сборка
echo.
echo Building project...
cmake --build build --config Release -j8
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

REM Запуск
echo.
echo ========================================
echo   Starting SynGT GUI...
echo ========================================
echo.

cd build\bin
if not exist "syngt_gui.exe" (
    echo ERROR: syngt_gui.exe not found in build\bin\
    echo Build may have failed.
    cd ..\..
    pause
    exit /b 1
)

syngt_gui.exe
cd ..\..

echo.
echo ========================================
echo   Application closed
echo ========================================
pause