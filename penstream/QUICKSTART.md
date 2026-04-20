# PenStream - Quick Start Guide

Get up and running in 5 minutes!

## Prerequisites

### For Server (Windows PC)
- Windows 10/11
- NVIDIA GPU (for NVENC encoding)
- Visual Studio 2022 (for building)

### For Client (Android)
- Android 8.0 or higher
- Android Studio (for building)
- Stylus support recommended

## Quick Build

### Option 1: Build Everything

```batch
cd penstream
build.bat
```

### Option 2: Build Separately

**Server only:**
```batch
cd penstream
scripts\build_server.bat
```

**Android only:**
```batch
cd penstream\android
gradlew.bat assembleDebug
```

## Running

### 1. Start the Server

```batch
cd penstream\build\server\Release
penstream_server.exe
```

You should see:
```
========================================
  PenStream Server v1.0.0
========================================
Port: 9696
Target: 1920x1080@60fps
Bitrate: 10000 kbps
Encoder: nvenc
========================================
[1/4] Initializing screen capture... OK (1920x1080)
[2/4] Initializing NVENC encoder... OK
[3/4] Initializing UDP transport... OK
[4/4] Initializing input handler... OK
========================================
Server started. Waiting for client...
```

### 2. Install Android App

**Via USB:**
```batch
adb install penstream\android\app\build\outputs\apk\debug\app-debug.apk
```

**Or transfer APK and install manually**

### 3. Connect

1. Ensure both devices are on the **same WiFi network**
2. Open PenStream app on Android
3. Wait for server discovery (or press Refresh)
4. Tap on your PC from the list
5. Press "Connect"

### 4. Start Drawing!

- Your Android screen now shows your PC desktop
- Touch/stylus input controls the PC
- Open any drawing app (Krita, Photoshop, etc.)

## Troubleshooting

### "No servers found"

- Check both devices are on same network
- Windows Firewall may be blocking - allow `penstream_server.exe`
- Try manual IP entry (Settings → Manual IP)

### High Latency / Lag

- Reduce resolution in Settings (1280x720 or 854x480)
- Use 5GHz WiFi instead of 2.4GHz
- Lower bitrate in Settings

### No Pressure Sensitivity

- Your device must support stylus pressure
- Some apps don't recognize virtual input - try Krita or Clip Studio Paint
- Full pressure support requires virtual HID driver (TODO)

### "Failed to initialize NVENC"

- NVIDIA driver not installed or outdated
- No NVIDIA GPU present
- Server will automatically try AMD/Intel fallback (if configured)

## Configuration

Edit `config.json` on the server:

```json
{
  "port": 9696,
  "width": 1920,
  "height": 1080,
  "fps": 60,
  "bitrate_kbps": 10000,
  "encoder": "nvenc",
  "low_latency": true
}
```

## Next Steps

- [Architecture Guide](docs/architecture.md) - Understand how it works
- [Contributing Guide](CONTRIBUTING.md) - How to contribute
- [Implementation Status](IMPLEMENTATION_STATUS.md) - What's done, what's next
