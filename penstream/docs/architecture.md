# PenStream Architecture

## System Overview

PenStream is a real-time screen streaming system optimized for ultra-low latency (<10ms) input feedback loop.

```
┌──────────────────────────────────────────────────────────────────┐
│                         WINDOWS PC                                │
│                                                                   │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │   Screen    │───▶│   DXGI      │───▶│   NVENC     │          │
│  │   Display   │    │   Capture   │    │   Encoder   │          │
│  └─────────────┘    └─────────────┘    └─────────────┘          │
│                              │                    │              │
│                              │                    ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │  SendInput  │◄───│   Input     │◄───│    UDP      │          │
│  │  (Virtual)  │    │   Handler   │    │   Server    │          │
│  └─────────────┘    └─────────────┘    └─────────────┘          │
│                                                  │               │
└──────────────────────────────────────────────────┼───────────────┘
                                                   │
                                                   │ UDP (WiFi)
                                                   │ Port 9696
                                                   ▼
┌──────────────────────────────────────────────────┼───────────────┐
│                    ANDROID DEVICE                │               │
│                                                  │               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │   Touch     │───▶│   Input     │───▶│    UDP      │          │
│  │   Screen    │    │   Capture   │    │   Client    │          │
│  └─────────────┘    └─────────────┘    └─────────────┘          │
│                              │                    │              │
│                              │                    ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │   OpenGL    │◄───│  MediaCodec │◄───│    UDP      │          │
│  │   Render    │    │   Decoder   │    │   Receive   │          │
│  └─────────────┘    └─────────────┘    └─────────────┘          │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

## Components

### Server (Windows)

#### 1. DXGI Capturer (`capture/dxgi_capturer.*`)

**Purpose:** Capture screen content using DirectX 11 Desktop Duplication API

**Key Features:**
- Zero-copy capture directly from GPU
- 60+ FPS capture rate
- Automatic resolution detection

**Data Flow:**
```
IDXGIOutputDuplication::AcquireNextFrame()
    ↓
ID3D11Texture2D (GPU memory)
    ↓
CopyResource() to staging texture
    ↓
Map() for CPU access (if needed by encoder)
```

#### 2. NVENC Encoder (`encode/nvenc_encoder.*`)

**Purpose:** Encode captured frames to H.264 using NVIDIA hardware

**Key Features:**
- D3D11 interop for zero-copy input
- Low-latency tuning (no B-frames, CBR)
- Adaptive bitrate support

**Encoder Settings:**
```cpp
presetGUID = NV_ENC_PRESET_P1_GUID;  // Fastest preset
tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
rateControlMode = NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
enableLookahead = 0;                  // No lookahead
frameIntervalP = 1;                   // No B-frames
gopLength = NV_ENC_INFINITE_GOPLENGTH;
```

#### 3. UDP Transport (`network/udp_transport.*`)

**Purpose:** Send encoded video and receive input packets

**Protocol:**
- Custom binary protocol over UDP
- 12-byte header + variable payload
- No retransmission (latency > reliability)

**Packet Types:**
| Type | ID | Direction | Description |
|------|----|-----------|-------------|
| VIDEO_FRAME | 0x01 | Server → Client | Encoded video |
| TOUCH_INPUT | 0x02 | Client → Server | Touch/stylus |
| HEARTBEAT | 0x03 | Bidirectional | Keep-alive |
| CONNECT_REQ | 0x10 | Client → Server | Connection request |
| CONNECT_RESP | 0x11 | Server → Client | Connection response |

#### 4. Input Handler (`input/input_handler.*`)

**Purpose:** Process incoming touch packets and simulate input

**Implementation:**
- Queues input events from network
- Calls VirtualInput for each event
- Thread-safe with mutex protection

#### 5. Virtual Input (`input/virtual_input.*`)

**Purpose:** Simulate mouse/stylus input using Windows SendInput API

**Current Limitations:**
- Only mouse emulation (no true pressure)
- For full pressure: Need virtual HID device (ViGEmBus)

### Client (Android)

#### 1. Network Client (`network_client.cpp`)

**Purpose:** Receive video packets and send input events

**Implementation:**
- UDP socket in background thread
- Frame callback to decoder
- Input packet construction

#### 2. Video Decoder (`video_decoder.cpp`)

**Purpose:** Decode H.264 video using MediaCodec

**Key Features:**
- Surface-based rendering (zero-copy to OpenGL)
- Low-latency mode
- Keyframe handling

#### 3. Renderer (`renderer.cpp`)

**Purpose:** Render decoded frames using OpenGL ES 3.0

**Implementation:**
- EGL surface management
- YUV → RGB conversion shader
- Full-screen quad rendering

#### 4. Input Capture (`input_capture.cpp`)

**Purpose:** Capture touch events and send to server

**Data Captured:**
- Position (x, y)
- Pressure (0.0 - 1.0)
- Tilt (if hardware supports)
- Button state

## Latency Budget

| Stage | Target | Typical |
|-------|--------|---------|
| Capture (DXGI) | <1ms | 0.5ms |
| Encode (NVENC) | <2ms | 1.5ms |
| Network (UDP) | <2ms | 1ms (5GHz WiFi) |
| Decode (MediaCodec) | <3ms | 2ms |
| Render (OpenGL) | <1ms | 0.5ms |
| Input Round-trip | <2ms | 1ms |
| **Total** | **<10ms** | **~6-7ms** |

## Memory Management

### Server

```
DXGI Frame Pool (3 frames)
    ↓
NVENC Input Texture (registered resource)
    ↓
NVENC Bitstream Buffer (output)
    ↓
UDP Send Buffer
```

### Client

```
UDP Receive Buffer
    ↓
MediaCodec Input Buffer
    ↓
MediaCodec Output Buffer (surface)
    ↓
OpenGL Texture
```

## Error Handling

### Connection Loss

1. Heartbeat timeout (5 seconds)
2. Auto-reconnect attempt
3. UI notification to user

### Packet Loss

- FEC (Forward Error Correction) - planned
- Jitter buffer for reordering
- Keyframe request on severe loss

### Encoder Failure

1. Log error
2. Attempt fallback encoder (NVENC → AMF → QSV)
3. Graceful shutdown if no encoder available

## Security Considerations

### Current Implementation

- Local network only (no internet exposure)
- Optional PIN for first connection
- No encryption (trusted network assumption)

### Future Improvements

- DTLS encryption option
- Certificate-based authentication
- Whitelist of known devices

## Performance Optimization

### Implemented

- Zero-copy capture → encode path
- Hardware encoding (NVENC)
- Surface-based decoding
- Background network thread

### TODO

- GPU-based color conversion (BGRA → NV12)
- Adaptive bitrate based on packet loss
- Frame dropping under load
- Multi-threaded encoding

## Testing Strategy

### Unit Tests

- Packet serialization/deserialization
- Config loading
- Input event processing

### Integration Tests

- Capture → encode → decode loop
- Network stress test (simulated packet loss)
- Input accuracy test (draw patterns, measure deviation)

### Manual Tests

- Different WiFi networks (2.4GHz, 5GHz)
- Various Android devices with stylus
- Drawing apps (Krita, Photoshop, Clip Studio)
