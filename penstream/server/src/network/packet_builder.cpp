#include "packet_builder.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace penstream::network {

std::vector<uint8_t> PacketBuilder::build_video_frame(
    uint32_t seq_num,
    uint32_t frame_id,
    uint16_t width,
    uint16_t height,
    const std::vector<uint8_t>& data)
{
    VideoFramePacket packet;
    packet.header.magic = PACKET_MAGIC;
    packet.header.version = PROTOCOL_VERSION;
    packet.header.type = static_cast<uint8_t>(PacketType::VIDEO_FRAME);
    packet.header.sequence_num = seq_num;
    packet.header.timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count());

    packet.frame_id = frame_id;
    packet.width = width;
    packet.height = height;
    packet.data_size = static_cast<uint32_t>(data.size());

    std::vector<uint8_t> buffer(sizeof(VideoFramePacket) + data.size());
    std::memcpy(buffer.data(), &packet, sizeof(VideoFramePacket));
    std::memcpy(buffer.data() + sizeof(VideoFramePacket), data.data(), data.size());

    return buffer;
}

std::vector<uint8_t> PacketBuilder::build_heartbeat(uint32_t seq_num) {
    PacketHeader header;
    header.magic = PACKET_MAGIC;
    header.version = PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(PacketType::HEARTBEAT);
    header.sequence_num = seq_num;
    header.timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count());

    std::vector<uint8_t> buffer(sizeof(PacketHeader));
    std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
    return buffer;
}

std::vector<uint8_t> PacketBuilder::build_connect_response(
    uint32_t seq_num,
    bool accepted,
    uint16_t width,
    uint16_t height,
    uint8_t codec,
    uint32_t bitrate_kbps)
{
    ConnectResponse packet;
    packet.header.magic = PACKET_MAGIC;
    packet.header.version = PROTOCOL_VERSION;
    packet.header.type = static_cast<uint8_t>(PacketType::CONNECT_RESP);
    packet.header.sequence_num = seq_num;
    packet.header.timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count());

    packet.accepted = accepted ? 1 : 0;
    packet.server_width = width;
    packet.server_height = height;
    packet.codec = codec;
    packet.bitrate_kbps = bitrate_kbps;

    std::vector<uint8_t> buffer(sizeof(ConnectResponse));
    std::memcpy(buffer.data(), &packet, sizeof(ConnectResponse));
    return buffer;
}

bool PacketBuilder::parse_input_packet(
    const uint8_t* data,
    size_t size,
    float& out_x,
    float& out_y,
    float& out_pressure,
    int8_t& out_tilt_x,
    int8_t& out_tilt_y,
    uint8_t& out_buttons)
{
    if (size < sizeof(InputPacket)) {
        return false;
    }

    const InputPacket* packet = reinterpret_cast<const InputPacket*>(data);

    if (!validate_header(&packet->header)) {
        return false;
    }

    if (packet->header.type != static_cast<uint8_t>(PacketType::TOUCH_INPUT)) {
        return false;
    }

    out_x = packet->x;
    out_y = packet->y;
    out_pressure = packet->pressure;
    out_tilt_x = packet->tilt_x;
    out_tilt_y = packet->tilt_y;
    out_buttons = packet->buttons;

    return true;
}

bool PacketBuilder::validate_header(const PacketHeader* header) {
    if (header->magic != PACKET_MAGIC) {
        spdlog::warn("Invalid packet magic: 0x{:04X}", header->magic);
        return false;
    }

    if (header->version != PROTOCOL_VERSION) {
        spdlog::warn("Unsupported protocol version: 0x{:02X}", header->version);
        return false;
    }

    return true;
}

} // namespace penstream::network
