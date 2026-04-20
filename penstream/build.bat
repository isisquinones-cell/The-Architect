@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   PenStream - Build All
echo ========================================
echo.

cd /d "%~dp0"

REM Check if building server, android, or both
set BUILD_TARGET=%1
if "%BUILD_TARGET%"=="" set BUILD_TARGET=all

if /i "%BUILD_TARGET%"=="server" (
    call scripts\build_server.bat
    goto :end
)

if /i "%BUILD_TARGET%"=="android" (
    call scripts\build_android.bat
    goto :end
)

if /i "%BUILD_TARGET%"=="all" (
    echo Building Server and Android...
    echo.
    echo [1/2] Building Server
    echo ========================================
    call scripts\build_server.bat
    if %ERRORLEVEL% neq 0 (
        echo.
        echo Server build failed!
        exit /b 1
    )

    echo.
    echo [2/2] Building Android
    echo ========================================
    call scripts\build_android.bat
    if %ERRORLEVEL% neq 0 (
        echo.
        echo Android build failed!
        exit /b 1
    )

    echo.
    echo ========================================
    echo   Build Complete!
    echo ========================================
    echo.
    echo Server: build\server\Release\penstream_server.exe
    echo Android: android\app\build\outputs\apk\debug\app-debug.apk
    goto :end
)

echo Usage: build.bat [server^|android^|all]
echo.
echo If no argument is provided, builds both server and android.

:end
pause
