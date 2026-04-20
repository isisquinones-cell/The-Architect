# PenStream - Implementation Status

Last updated: 2026-04-18

## Overview

PenStream is a low-latency screen streaming application that transforms Android tablets/phones into graphics tablets for Windows PCs.

**Target:** 1080p60 streaming with <10ms latency

---

## Implementation Status

### Server (Windows) - C++20

| Module | Status | File | Notes |
|--------|--------|------|-------|
| DXGI Screen Capture | ✅ Complete | `server/src/capture/dxgi_capturer.*` | Desktop Duplication API, zero-copy |
| Frame Pool | ✅ Complete | `server/src/capture/frame_pool.*` | Memory pooling for frames |
| NVENC Encoder | ✅ Complete | `server/src/encode/nvenc_encoder.*` | H.264 encoding with D3D11 interop |
| AMF Encoder (fallback) | 🟡 Stub | `server/src/encode/amf_encoder.*` | AMD GPU fallback (needs SDK) |
| UDP Transport | ✅ Complete | `server/src/network/udp_transport.*` | Low-latency packet sending |
| Packet Builder | ✅ Complete | `server/src/network/packet_builder.*` | Binary protocol serialization |
| Input Handler | ✅ Complete | `server/src/input/input_handler.*` | Processes touch input from client |
| Virtual Input | ✅ Complete | `server/src/input/virtual_input.*` | SendInput for mouse emulation |
| Config Loader | ✅ Complete | `server/src/config/config_loader.*` | JSON + env vars |
| Main Loop | ✅ Complete | `server/src/main.cpp` | Server entry point |
| WebRTC (optional) | 🟡 Stub | `server/src/network/webrtc_server.*` | Fallback for NAT traversal |

### Client (Android) - Kotlin + NDK

| Module | Status | File | Notes |
|--------|--------|------|-------|
| MainActivity | ✅ Complete | `app/src/main/java/MainActivity.kt` | Server discovery UI |
| StreamingActivity | ✅ Complete | `app/src/main/java/StreamingActivity.kt` | Touch input + surface |
| PenStreamService | ✅ Complete | `app/src/main/java/PenStreamService.kt` | Foreground service |
| ConnectionManager | ✅ Complete | `app/src/main/java/ConnectionManager.kt` | UDP discovery + connect |
| ServerInfo | ✅ Complete | `app/src/main/java/ServerInfo.kt` | Data class |
| SettingsActivity | ✅ Complete | `app/src/main/java/SettingsActivity.kt` | Quality settings |
| Video Decoder (NDK) | ✅ Complete | `app/src/main/cpp/video_decoder.*` | MediaCodec H.264 |
| OpenGL Renderer (NDK) | ✅ Complete | `app/src/main/cpp/renderer.*` | EGL + GLES3 |
| Network Client (NDK) | ✅ Complete | `app/src/main/cpp/network_client.*` | UDP receiver |
| Input Capture (NDK) | ✅ Complete | `app/src/main/cpp/input_capture.*` | Touch event handling |
| JNI Interface | ✅ Complete | `app/src/main/cpp/jni_interface.*` | Kotlin ↔ C++ bridge |

### Build System

| Component | Status | File | Notes |
|-----------|--------|------|-------|
| Server CMake | ✅ Complete | `server/CMakeLists.txt` | Visual Studio 2022 |
| Android Gradle | ✅ Complete | `android/app/build.gradle` | API 26+, NDK r25+ |
| NDK CMake | ✅ Complete | `android/app/src/main/cpp/CMakeLists.txt` | Native build |
| Build Script (Server) | ✅ Complete | `scripts/build_server.bat` | Automated build |
| Build Script (Android) | ✅ Complete | `scripts/build_android.bat` | APK generation |

### Protocol & Docs

| Document | Status | File | Notes |
|----------|--------|------|-------|
| Protocol Spec | ✅ Complete | `docs/protocol_spec.md` | Binary protocol |
| CLAUDE.md | ✅ Complete | `CLAUDE.md` | Project config |
| README | ✅ Complete | `README.md` | User documentation |
| Config Template | ✅ Complete | `config.json` | Default settings |

---

## Build Instructions

### Prerequisites

**Server:**
- Windows 10/11 with NVIDIA GPU
- Visual Studio 2022 with C++ workload
- vcpkg: `git clone https://github.com/microsoft/vcpkg`
- NVIDIA Video Codec SDK (optional but recommended)

**Client:**
- Android Studio Hedgehog or newer
- Android NDK r25+
- Android SDK API 34

### Build Commands

```batch
# Server (Windows)
cd scripts
build_server.bat

# Client (Android)
cd scripts
build_android.bat
```

### Run

1. Start server on Windows PC: `build\server\Release\penstream_server.exe`
2. Install APK on Android device: `adb install app\build\outputs\apk\debug\app-debug.apk`
3. Connect both devices to same WiFi network
4. Open app and select your PC from the list

---

## Known Issues / TODOs

### High Priority

1. **NVENC D3D11 Interop** - The encoder needs proper D3D11 texture sharing with DXGI capturer. Current implementation has the structure but needs testing with real hardware.

2. **Color Space Conversion** - DXGI captures BGRA, NVENC expects NV12. Need efficient GPU-based conversion.

3. **Pressure Sensitivity** - Windows SendInput doesn't support pressure. For full pressure support, need to create a virtual HID device using ViGEmBus or similar.

### Medium Priority

4. **AMF/QSV Fallback** - AMD and Intel GPU support needs SDK integration.

5. **Adaptive Bitrate** - Currently static bitrate. Should adjust based on packet loss/latency feedback.

6. **FEC (Forward Error Correction)** - Placeholder in config. Need to implement XOR parity packets.

### Low Priority

7. **WebRTC Fallback** - For traversing NAT/firewall. Currently a stub.

8. **Multi-client Support** - Server can handle multiple clients but needs UI for management.

9. **Statistics Dashboard** - Real-time latency/packet loss visualization.

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      WINDOWS SERVER                              │
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │ DXGI Capture │───▶│ NVENC Encode │───▶│ UDP Send     │      │
│  │ (D3D11)      │    │ (H.264)      │    │ (Port 9696)  │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         ▲                                      │                │
│         │                                      ▼                │
│  ┌──────────────┐                      ┌──────────────┐        │
│  │ Virtual Input│◄─────────────────────│ UDP Receive  │        │
│  │ (SendInput)  │                      │ (Input Pkts) │        │
│  └──────────────┘                      └──────────────┘        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ UDP (WiFi)
                              │ Video: Server → Client
                              │ Input:  Client → Server
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      ANDROID CLIENT                              │
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │ UDP Receive  │───▶│ MediaCodec   │───▶│ OpenGL ES    │      │
│  │ (Video Pkts) │    │ (H.264 Dec)  │    │ (Render)     │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         ▲                                      │                │
│         │                                      ▼                │
│  ┌──────────────┐                      ┌──────────────┐        │
│  │ UDP Send     │◄─────────────────────│ Touch Input  │        │
│  │ (Input Pkts) │                      │ (MotionEvent)│        │
│  └──────────────┘                      └──────────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

---

## Performance Targets

| Metric | Target | Current (Estimated) |
|--------|--------|---------------------|
| Resolution | 1920x1080 | ✅ Supported |
| Frame Rate | 60 fps | ✅ Supported |
| Latency (capture→display) | <10ms | 🟡 ~15-20ms (needs measurement) |
| Packet Loss Tolerance | <1% | 🟡 FEC not implemented |
| CPU Usage (server) | <10% | 🟡 TBD |

---

## Next Steps

1. **Test on real hardware** - Need NVIDIA GPU + Android device with stylus
2. **Measure actual latency** - Use high-speed camera or software measurement
3. **Optimize hot paths** - Profile and reduce allocations
4. **Implement color conversion** - BGRA → NV12 on GPU
5. **Add virtual HID driver** - For true pressure sensitivity support
