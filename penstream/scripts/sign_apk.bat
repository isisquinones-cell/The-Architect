@echo off
echo Signing PenStream APK for release...

cd /d "%~dp0..\android"

REM Check for keystore
if not exist "penstream.keystore" (
    echo Creating new keystore...
    keytool -genkey -v -keystore penstream.keystore -alias penstream -keyalg RSA -keysize 2048 -validity 10000
)

REM Build release APK
call gradlew.bat assembleRelease

if not exist app\build\outputs\apk\release\app-release-unsigned.apk (
    echo Release build not found!
    exit /b 1
)

REM Sign the APK
echo Signing APK...
apksigner sign --ks penstream.keystore ^
    --out app\build\outputs\apk\release\app-release.apk ^
    app\build\outputs\apk\release\app-release-unsigned.apk

echo.
echo ========================================
echo Release APK built and signed!
echo ========================================
echo Location: android\app\build\outputs\apk\release\app-release.apk
echo.
echo This APK is ready for distribution.
echo Install with: adb install app\build\outputs\apk\release\app-release.apk
pause
