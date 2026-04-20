@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   PenStream Android Build Script
echo ========================================
echo.

cd /d "%~dp0..\android"

REM Check for Android Studio
echo [1/4] Checking prerequisites...

set "ANDROID_HOME="
set "ANDROID_SDK_ROOT="

REM Try to find Android SDK
for %%i in (
    "%LOCALAPPDATA%\Android\Sdk"
    "%ProgramFiles%\Android\Sdk"
    "%ProgramFiles(x86)%\Android\Sdk"
) do (
    if exist "%%i" (
        set "ANDROID_HOME=%%i"
        set "ANDROID_SDK_ROOT=%%i"
        goto :found_sdk
    )
)

if not defined ANDROID_HOME (
    echo ERROR: Android SDK not found
    echo Please install Android Studio or set ANDROID_HOME and ANDROID_SDK_ROOT
    pause
    exit /b 1
)

:found_sdk
echo Found Android SDK: %ANDROID_HOME%

REM Check for NDK
set "NDK_PATH="
for %%i in (
    "%ANDROID_HOME%\ndk\*"
) do (
    if exist "%%i" (
        set "NDK_PATH=%%i"
        goto :found_ndk
    )
)

if not defined NDK_PATH (
    echo ERROR: Android NDK not found
    echo Please install NDK via Android Studio SDK Manager
    pause
    exit /b 1
)

:found_ndk
echo Found NDK: %NDK_PATH%

REM Check for Gradle
set "GRADLE_WRAPPER=gradlew.bat"
if not exist "%GRADLE_WRAPPER%" (
    echo ERROR: Gradle wrapper not found
    echo Make sure you're running this script from the android directory
    pause
    exit /b 1
)

REM Set environment variables
set "ANDROID_NDK_HOME=%NDK_PATH%"
set "ANDROID_NDK_ROOT=%NDK_PATH%"

echo.
echo [2/4] Syncing Gradle...
call gradlew.bat --version

REM Clean previous build
echo.
echo [3/4] Cleaning previous build...
call gradlew.bat clean

REM Build APK
echo.
echo [4/4] Building APK...
call gradlew.bat assembleDebug

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed
    echo Check the error messages above for details
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build successful!
echo ========================================
echo.
echo APK location:
for /f "delims=" %%i in ('dir /b /o-d app\build\outputs\apk\debug\app-*-debug.apk 2^>nul') do (
    echo   app\build\outputs\apk\debug\%%i
    set "APK_PATH=app\build\outputs\apk\debug\%%i"
    goto :found_apk
)

:found_apk
echo.
echo To install on device:
echo adb install -r "%APK_PATH%"
echo.
echo To install and run:
echo adb install -r "%APK_PATH%" ^&^& adb shell am start -n com.penstream.app/.MainActivity
echo.
pause
