#include "network_client.h"
#include <android/log.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define LOG_TAG "PenStreamNetwork"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace penstream::android {

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint32_t sequence_num;
    uint32_t timestamp;
};

struct VideoFramePacket {
    PacketHeader header;
    uint32_t frame_id;
    uint16_t width;
    uint16_t height;
    uint32_t data_size;
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
#pragma pack(pop)

constexpr uint16_t PACKET_MAGIC = 0x5053;
constexpr uint8_t PACKET_VIDEO = 0x01;
constexpr uint8_t PACKET_INPUT = 0x02;

NetworkClient::NetworkClient()
    : m_server_port(9696)
    , m_running(false)
    , m_socket(-1)
    , m_sequence_num(0)
{}

NetworkClient::~NetworkClient() {
    stop();
}

void NetworkClient::set_frame_callback(std::function<void(const std::vector<uint8_t>&)> callback) {
    m_frame_callback = callback;
}

bool NetworkClient::connect(const std::string& address, int port) {
    m_server_address = address;
    m_server_port = port;

    // Create UDP socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket < 0) {
        LOGE("Failed to create socket");
        return false;
    }

    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    LOGI("Connected to %s:%d", address.c_str(), port);
    return true;
}

void NetworkClient::start() {
    if (m_running) {
        return;
    }

    m_running = true;
    m_receive_thread = std::thread(&NetworkClient::receive_loop, this);
}

void NetworkClient::stop() {
    if (!m_running) {
        return;
    }

    m_running = false;

    if (m_receive_thread.joinable()) {
        m_receive_thread.join();
    }

    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }

    LOGI("Disconnected");
}

void NetworkClient::send_input(float x, float y, float pressure,
                                int8_t tilt_x, int8_t tilt_y, uint8_t buttons) {
    if (m_socket < 0 || !m_running) {
        return;
    }

    InputPacket packet;
    packet.header.magic = PACKET_MAGIC;
    packet.header.version = 1;
    packet.header.type = PACKET_INPUT;
    packet.header.sequence_num = m_sequence_num++;
    packet.header.timestamp = 0;

    packet.x = x;
    packet.y = y;
    packet.pressure = pressure;
    packet.tilt_x = tilt_x;
    packet.tilt_y = tilt_y;
    packet.buttons = buttons;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_server_port);
    inet_pton(AF_INET, m_server_address.c_str(), &server_addr.sin_addr);

    sendto(m_socket, &packet, sizeof(packet), 0,
           reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
}

void NetworkClient::receive_loop() {
    std::vector<uint8_t> buffer(65536); // Max UDP packet

    while (m_running) {
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        ssize_t received = recvfrom(
            m_socket,
            buffer.data(),
            buffer.size(),
            0,
            reinterpret_cast<sockaddr*>(&from_addr),
            &from_len
        );

        if (received < 0) {
            continue; // Timeout, keep listening
        }

        if (received < sizeof(PacketHeader)) {
            continue;
        }

        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer.data());

        if (header->magic != PACKET_MAGIC) {
            continue;
        }

        if (header->type == PACKET_VIDEO) {
            // Extract video data
            const VideoFramePacket* video = reinterpret_cast<const VideoFramePacket*>(buffer.data());
            const uint8_t* data_start = buffer.data() + sizeof(VideoFramePacket);

            std::vector<uint8_t> video_data(
                data_start,
                data_start + video->data_size
            );

            if (m_frame_callback) {
                m_frame_callback(video_data);
            }
        }
    }
}

} // namespace penstream::android
