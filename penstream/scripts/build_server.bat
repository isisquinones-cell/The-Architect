@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   PenStream Server Build Script
echo ========================================
echo.

cd /d "%~dp0.."

REM Check for Visual Studio
echo [1/4] Checking prerequisites...
set "VS_PATH="
for %%i in (
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist "%%i" (
        set "VS_PATH=%%i"
        goto :found_vs
    )
)

echo ERROR: Visual Studio 2022 not found
echo Please install Visual Studio 2022 with C++ workload
pause
exit /b 1

:found_vs
echo Found Visual Studio

REM Check for vcpkg
if defined VCPKG_ROOT (
    echo Found vcpkg: %VCPKG_ROOT%
) else (
    echo ERROR: VCPKG_ROOT not set
    echo Please install vcpkg and set VCPKG_ROOT environment variable
    echo https://github.com/microsoft/vcpkg
    pause
    exit /b 1
)

REM Check for NVIDIA Video Codec SDK
if defined NVENC_SDK_PATH (
    echo Found NVENC SDK: %NVENC_SDK_PATH%
) else (
    echo WARNING: NVENC_SDK_PATH not set
    echo Download from: https://developer.nvidia.com/nvidia-video-codec-sdk
    echo Building without NVENC support...
)

REM Install dependencies
echo.
echo [2/4] Installing dependencies...
%VCPKG_ROOT%\vcpkg install spdlog:x64-windows nlohmann-json:x64-windows

REM Create build directory
echo.
echo [3/4] Configuring build...
if not exist build\server mkdir build\server
cd build\server

REM Configure with CMake
call "%VS_PATH%" >nul 2>&1

cmake ..\..\server -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_INSTALL_PREFIX=%CD%\install ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build
echo.
echo [4/4] Building...
cmake --build . --config Release -- /m

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build successful!
echo ========================================
echo.
echo Executable: build\server\Release\penstream_server.exe
echo.
echo Next steps:
echo 1. Copy penstream_server.exe to your PC
echo 2. Run penstream_server.exe
echo 3. Install the Android APK on your device
echo 4. Connect both devices to the same WiFi
echo.
pause
