# Contributing to PenStream

Thanks for considering contributing to PenStream! This guide will help you get started.

## Development Setup

### Prerequisites

**For Server Development:**
- Windows 10/11 with NVIDIA GPU (for NVENC testing)
- Visual Studio 2022 with C++ workload
- vcpkg package manager
- NVIDIA Video Codec SDK (optional, for NVENC)

**For Android Development:**
- Android Studio Hedgehog or newer
- Android NDK r25+
- Android device with stylus support (recommended for testing)

### Building

```batch
# Build everything
build.bat

# Build server only
build.bat server

# Build Android only
build.bat android
```

### Running Tests

```batch
# Server tests (after building with ENABLE_TESTS=ON)
cd build\server
ctest --output-on-failure

# Android tests
adb shell am instrument -w com.penstream.app.test/androidx.test.runner.AndroidJUnitRunner
```

## Code Style

### C++ (Server & NDK)

- **Standard:** C++20
- **Formatting:** Use `clang-format` with default LLVM style
- **Naming:** 
  - Classes: `PascalCase`
  - Functions: `snake_case`
  - Members: `m_` prefix + `snake_case`
  - Constants: `UPPER_SNAKE_CASE`

```cpp
// Example
class DXGICapturer {
public:
    bool initialize();
    bool capture_frame(Frame& out_frame);

private:
    ID3D11Device* m_device;
    bool m_initialized;
};
```

### Kotlin (Android)

- **Style:** Follow [Kotlin coding conventions](https://kotlinlang.org/docs/coding-conventions.html)
- **Naming:**
  - Classes: `PascalCase`
  - Functions/Properties: `camelCase`
  - Constants: `SCREAMING_SNAKE_CASE`

```kotlin
// Example
class PenStreamService : Service() {
    private var serverAddress: String = ""
    
    fun startStreaming() {
        // Implementation
    }
}
```

## Architecture

### Server Data Flow

```
DXGI Capture → NVENC Encode → UDP Send
     ↑                        ↓
Virtual Input ← UDP Receive ← Client
```

### Client Data Flow

```
UDP Receive → MediaCodec Decode → OpenGL Render
     ↑                              ↓
UDP Send ← Touch Input (MotionEvent)
```

## Key Design Decisions

1. **Zero-copy where possible:** DXGI capture directly to NVENC input texture
2. **No GC in hot paths:** All C++ code avoids allocations during streaming
3. **Latency over quality:** Prefer lower resolution over buffering
4. **Local-first:** No cloud services, everything on local network

## Pull Request Process

1. **Fork** the repository
2. **Create a branch** for your feature (`feature/my-feature`)
3. **Make changes** following the code style
4. **Test** on real hardware if possible
5. **Submit PR** with description of changes

## Issues

When reporting issues, please include:

- **OS version** (Windows 10/11, Android version)
- **GPU model** (NVIDIA/AMD/Intel)
- **Steps to reproduce**
- **Logs** from the application

### Finding Logs

**Server:**
```
build\server\penstream_server.log
```

**Android:**
```
adb logcat | grep PenStream
```

## Performance Targets

| Metric | Target | How to Measure |
|--------|--------|----------------|
| Latency | <10ms | High-speed camera or software measurement |
| FPS | 60 stable | Server console output |
| Packet Loss | <1% | Network stats overlay |

## Areas for Contribution

### High Priority

1. **AMD/Intel GPU Support:** AMF and QSV encoder implementations
2. **Virtual HID Driver:** For true pressure sensitivity on Windows
3. **Adaptive Bitrate:** Adjust based on network conditions
4. **FEC Implementation:** Forward error correction for packet loss

### Medium Priority

1. **WebRTC Fallback:** For NAT traversal
2. **Multi-client UI:** Manage multiple connected devices
3. **Statistics Dashboard:** Real-time performance visualization
4. **Documentation:** Improve setup guides and troubleshooting

## Questions?

Feel free to open an issue for any questions about contributing!
