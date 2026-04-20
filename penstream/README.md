# PenStream

Transform your Android tablet or phone into a high-performance graphics tablet with ultra-low latency.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Android-green.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Android API](https://img.shields.io/badge/API-26%2B-green.svg)

## Features

- **1080p60 streaming** with <10ms target latency
- **DirectX 11 capture** for zero-copy screen capture
- **NVENC hardware encoding** for minimal CPU usage
- **Pressure sensitivity** support (4096 levels)
- **Automatic discovery** - no manual IP configuration
- **Cross-platform input** - works with any drawing app

## Quick Start

### Build

```batch
# Build everything
build.bat

# Or build separately
scripts\build_server.bat    # Windows server
scripts\build_android.bat   # Android APK
```

### Run

1. Start `penstream_server.exe` on your Windows PC
2. Install the APK on your Android device
3. Connect both devices to the same WiFi network
4. Open the app and select your PC from the list

See [QUICKSTART.md](QUICKSTART.md) for detailed instructions.

## Architecture

```
┌─────────────────┐         UDP          ┌─────────────────┐
│  Windows Server │◄────────────────────►│  Android Client │
│                 │      Port 9696       │                 │
│  DXGI Capture   │────┐                 │  MediaCodec     │
│  NVENC Encode   │────┼───── Video ────►│  OpenGL Render  │
│  Input Handler  │◄───┘                 │  Input Capture  │
└─────────────────┘                      └─────────────────┘
       ▲                                        │
       └──────────────── Input ─────────────────┘
```

See [docs/architecture.md](docs/architecture.md) for full details.

## Prerequisites

### Server (Windows)
- Windows 10/11 with NVIDIA GPU (for NVENC)
- Visual Studio 2022 with C++ workload
- vcpkg package manager
- NVIDIA Video Codec SDK (optional but recommended)

### Client (Android)
- Android 8.0 (API 26) or higher
- Android Studio with NDK r25+
- Device with stylus support (recommended)

## Project Structure

```
penstream/
├── server/                     # Windows server (C++20)
│   ├── src/
│   │   ├── capture/           # DXGI screen capture
│   │   ├── encode/            # NVENC/AMF encoding
│   │   ├── network/           # UDP transport + protocol
│   │   └── input/             # Virtual input handling
│   └── CMakeLists.txt
├── android/                    # Android client (Kotlin + NDK)
│   └── app/
│       ├── src/main/cpp/      # Native code (NDK)
│       └── src/main/java/     # Kotlin UI
├── docs/                       # Documentation
├── scripts/                    # Build scripts
├── build.bat                   # Root build script
└── config.json                 # Server configuration
```

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

## Performance Targets

| Metric | Target | Status |
|--------|--------|--------|
| Resolution | 1920x1080 | ✅ |
| Frame Rate | 60 fps | ✅ |
| Latency | <10ms | 🟡 (~15ms estimated) |
| Packet Loss | <1% | 🟡 (FEC pending) |

See [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) for details.

## Documentation

- [Quick Start Guide](QUICKSTART.md) - Get started in 5 minutes
- [Architecture](docs/architecture.md) - System design and data flow
- [Protocol Spec](docs/protocol_spec.md) - Binary protocol documentation
- [Contributing](CONTRIBUTING.md) - How to contribute
- [Implementation Status](IMPLEMENTATION_STATUS.md) - What's done

## Troubleshooting

**No servers found**
- Ensure both devices are on the same network
- Check Windows Firewall is not blocking UDP port 9696

**High latency**
- Reduce resolution or bitrate in settings
- Use 5GHz WiFi instead of 2.4GHz
- Ensure NVENC is being used (check server logs)

**No pressure sensitivity**
- Your device must support stylus pressure
- Some apps may not recognize virtual input - try Krita or Clip Studio

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Credits

Built with:
- DirectX 11 Desktop Duplication API
- NVIDIA NVENC
- Android MediaCodec
- OpenGL ES 3.0

## Acknowledgments

- [NVIDIA Video Codec SDK](https://developer.nvidia.com/video-codec-sdk)
- [Android NDK](https://developer.android.com/ndk)
- [vcpkg](https://github.com/microsoft/vcpkg)
