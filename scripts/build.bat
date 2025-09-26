@echo off
REM ============================================================
REM  SynGT C++ - Build Script for Windows
REM ============================================================

setlocal

set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build
set BUILD_TYPE=Release

echo.
echo ============================================================
echo   SynGT C++ Build Script
echo ============================================================
echo.

REM
:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="clean" (
    set CLEAN_BUILD=1
    shift
    goto parse_args
)
if /i "%~1"=="debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%~1"=="release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if /i "%~1"=="help" (
    goto show_help
)
shift
goto parse_args
:end_parse

REM
if defined CLEAN_BUILD (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
        echo Build directory cleaned
    )
)

REM
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

REM
cd /d "%BUILD_DIR%"

REM
echo.
echo Configuring CMake (%BUILD_TYPE%)...
cmake -G Ninja ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_CLI=ON ^
    -DBUILD_GUI=OFF ^
    -DBUILD_TESTS=ON ^
    -DBUILD_EXAMPLES=OFF ^
    ..

if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

echo CMake configured successfully

REM
echo.
echo Building project...
ninja

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo ============================================================
echo Build completed successfully!
echo ============================================================
echo.
echo Executable location:
echo   %BUILD_DIR%\bin\syngt_cli.exe
echo.
echo To run:
echo   cd build\bin
echo   syngt_cli.exe --help
echo.

exit /b 0

:show_help
echo.
echo Usage: build.bat [options]
echo.
echo Options:
echo   clean          Clean build directory before building
echo   debug          Build in Debug mode
echo   release        Build in Release mode (default)
echo   help           Show this help
echo.
echo Examples:
echo   build.bat                    # Build in Release mode
echo   build.bat clean              # Clean and build
echo   build.bat debug              # Build in Debug mode
echo   build.bat clean release      # Clean and build Release
echo.
exit /b 0