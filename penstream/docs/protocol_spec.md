# PenStream Protocol Specification

## Overview

PenStream uses a custom binary protocol over UDP for low-latency video streaming and input transmission.

## Packet Structure

### Header (12 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 2 | Magic | `0x5053` ("PS") |
| 2 | 1 | Version | Protocol version (currently `0x01`) |
| 3 | 1 | Type | Packet type identifier |
| 4 | 4 | Sequence | Packet sequence number |
| 8 | 4 | Timestamp | Unix timestamp (milliseconds) |

### Packet Types

| ID | Name | Direction | Description |
|----|------|-----------|-------------|
| `0x01` | VIDEO_FRAME | Server → Client | Encoded video frame |
| `0x02` | TOUCH_INPUT | Client → Server | Touch/stylus input |
| `0x03` | HEARTBEAT | Bidirectional | Keep-alive packet |
| `0x10` | CONNECT_REQ | Client → Server | Connection request |
| `0x11` | CONNECT_RESP | Server → Client | Connection response |
| `0x12` | CONNECT_ACK | Client → Server | Connection acknowledgment |
| `0x20` | STATS | Bidirectional | Performance statistics |

## Packet Details

### VIDEO_FRAME

```
Header (12 bytes)
frame_id (4 bytes)
width (2 bytes)
height (2 bytes)
data_size (4 bytes)
H.264 NAL data (data_size bytes)
```

### TOUCH_INPUT

```
Header (12 bytes)
x (4 bytes, float)     - Normalized X position (0.0-1.0)
y (4 bytes, float)     - Normalized Y position (0.0-1.0)
pressure (4 bytes, float) - Pressure (0.0-1.0)
tilt_x (1 byte, int8)  - Tilt X (-90 to 90)
tilt_y (1 byte, int8)  - Tilt Y (-90 to 90)
buttons (1 byte, uint8) - Button bitmask
padding (1 byte)
```

### CONNECT_REQ

```
Header (12 bytes)
client_name (32 bytes) - UTF-8 device name
max_width (2 bytes)    - Max supported width
max_height (2 bytes)   - Max supported height
codecs (1 byte)        - Codec bitmask (bit 0 = H.264)
```

### CONNECT_RESP

```
Header (12 bytes)
accepted (1 byte)      - 1 = accepted, 0 = rejected
server_width (2 bytes) - Server screen width
server_height (2 bytes) - Server screen height
codec (1 byte)         - Selected codec
bitrate_kbps (4 bytes) - Target bitrate
```

## Connection Flow

```
Client                          Server
  |                               |
  |--- Broadcast (HEARTBEAT) ---->|
  |                               |
  |<-- CONNECT_RESP (unicast) ---|
  |                               |
  |--- CONNECT_REQ -------------->|
  |                               |
  |<-- CONNECT_RESP -------------|
  |                               |
  |--- CONNECT_ACK -------------->|
  |                               |
  |<== Video stream starts =====>|
  |--- Input packets ------------>|
  |                               |
  |<-- STATS (periodic) ---------|
  |--- STATS (periodic) -------->|
```

## Error Handling

- **Invalid magic**: Packet silently dropped
- **Version mismatch**: Server sends CONNECT_RESP with accepted=0
- **Packet loss**: Handled by FEC (Forward Error Correction)
- **Timeout**: If no heartbeat for 5 seconds, connection considered lost

## Security

- Optional PIN authentication (sent in CONNECT_REQ)
- Local network only (no internet exposure by default)
- DTLS encryption can be enabled for sensitive environments
