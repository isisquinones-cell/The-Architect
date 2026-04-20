#pragma once

#include <cstdint>
#include <vector>
#include <chrono>

namespace penstream::network {

#pragma pack(push, 1)

struct PacketHeader {
    uint16_t magic;         // 0x5053 = "PS"
    uint8_t version;        // Protocol version
    uint8_t type;           // Packet type
    uint32_t sequence_num;  // Sequence number
    uint32_t timestamp;     // Timestamp (ms)
};

struct VideoFramePacket {
    PacketHeader header;
    uint32_t frame_id;
    uint16_t width;
    uint16_t height;
    uint32_t data_size;
    // Followed by data_size bytes of H.264 data
};

struct InputPacket {
    PacketHeader header;
    float x;
    float y;
    float pressure;
    int8_t tilt_x;
    int8_t tilt_y;
    uint8_t buttons;
};

struct ConnectRequest {
    PacketHeader header;
    char client_name[32];
    uint16_t max_width;
    uint16_t max_height;
    uint8_t supported_codecs;
};

struct ConnectResponse {
    PacketHeader header;
    uint8_t accepted;
    uint16_t server_width;
    uint16_t server_height;
    uint8_t codec;
    uint32_t bitrate_kbps;
};

#pragma pack(pop)

// Packet types
enum class PacketType : uint8_t {
    VIDEO_FRAME = 0x01,
    TOUCH_INPUT = 0x02,
    HEARTBEAT = 0x03,
    CONNECT_REQ = 0x10,
    CONNECT_RESP = 0x11,
    CONNECT_ACK = 0x12,
    STATS = 0x20,
};

constexpr uint16_t PACKET_MAGIC = 0x5053; // "PS"
constexpr uint8_t PROTOCOL_VERSION = 0x01;

class PacketBuilder {
public:
    static std::vector<uint8_t> build_video_frame(
        uint32_t seq_num,
        uint32_t frame_id,
        uint16_t width,
        uint16_t height,
        const std::vector<uint8_t>& data);

    static std::vector<uint8_t> build_heartbeat(uint32_t seq_num);

    static std::vector<uint8_t> build_connect_response(
        uint32_t seq_num,
        bool accepted,
        uint16_t width,
        uint16_t height,
        uint8_t codec,
        uint32_t bitrate_kbps);

    static bool parse_input_packet(
        const uint8_t* data,
        size_t size,
        float& out_x,
        float& out_y,
        float& out_pressure,
        int8_t& out_tilt_x,
        int8_t& out_tilt_y,
        uint8_t& out_buttons);

    static bool validate_header(const PacketHeader* header);
};

} // namespace penstream::network
