#include "udp_transport.h"
#include "packet_builder.h"
#include <spdlog/spdlog.h>

namespace penstream::network {

UDPTransport::UDPTransport(uint16_t port)
    : m_socket(INVALID_SOCKET)
    , m_port(port)
    , m_has_client(false)
    , m_sequence_num(0)
{
    memset(&m_server_addr, 0, sizeof(m_server_addr));
    memset(&m_client_addr, 0, sizeof(m_client_addr));
    memset(&m_stats, 0, sizeof(m_stats));
}

UDPTransport::~UDPTransport() {
    shutdown();
}

bool UDPTransport::initialize() {
    // Initialize Winsock
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        spdlog::error("WSAStartup failed: {}", result);
        return false;
    }

    if (!create_socket()) {
        WSACleanup();
        return false;
    }

    if (!bind_socket()) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    spdlog::info("UDP transport initialized on port {}", m_port);
    return true;
}

bool UDPTransport::create_socket() {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        spdlog::error("Failed to create socket: {}", WSAGetLastError());
        return false;
    }

    // Set buffer size for high throughput
    int send_buf_size = 256 * 1024; // 256 KB
    int recv_buf_size = 256 * 1024;
    setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF,
               reinterpret_cast<char*>(&send_buf_size), sizeof(send_buf_size));
    setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF,
               reinterpret_cast<char*>(&recv_buf_size), sizeof(recv_buf_size));

    // Enable broadcast for discovery
    int broadcast = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<char*>(&broadcast), sizeof(broadcast));

    return true;
}

bool UDPTransport::bind_socket() {
    memset(&m_server_addr, 0, sizeof(m_server_addr));
    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_server_addr.sin_port = htons(m_port);

    int result = bind(m_socket, reinterpret_cast<sockaddr*>(&m_server_addr),
                      sizeof(m_server_addr));

    if (result == SOCKET_ERROR) {
        spdlog::error("Failed to bind socket: {}", WSAGetLastError());
        return false;
    }

    return true;
}

void UDPTransport::shutdown() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    WSACleanup();
    spdlog::info("UDP transport shutdown");
}

bool UDPTransport::send_to(const sockaddr_in& addr, const std::vector<uint8_t>& data) {
    if (m_socket == INVALID_SOCKET) {
        return false;
    }

    int result = sendto(m_socket, reinterpret_cast<const char*>(data.data()),
                        static_cast<int>(data.size()), 0,
                        reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

    if (result == SOCKET_ERROR) {
        spdlog::warn("sendto failed: {}", WSAGetLastError());
        return false;
    }

    m_stats.packets_sent++;
    m_stats.bytes_sent += result;
    return true;
}

bool UDPTransport::receive(std::vector<uint8_t>& out_data, sockaddr_in& out_addr, int timeout_ms) {
    if (m_socket == INVALID_SOCKET) {
        return false;
    }

    // Set timeout
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms * 1000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<char*>(&tv), sizeof(tv));

    out_data.resize(4096); // Max packet size
    int addr_len = sizeof(out_addr);

    int result = recvfrom(m_socket, reinterpret_cast<char*>(out_data.data()),
                          static_cast<int>(out_data.size()), 0,
                          reinterpret_cast<sockaddr*>(&out_addr), &addr_len);

    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAETIMEDOUT && err != WSAEWOULDBLOCK) {
            spdlog::warn("recvfrom failed: {}", err);
        }
        return false;
    }

    out_data.resize(result);
    m_stats.packets_received++;
    m_stats.bytes_received += result;
    return true;
}

bool UDPTransport::send_video_frame(const std::vector<uint8_t>& encoded_data,
                                     uint32_t frame_id, uint64_t timestamp_us,
                                     uint16_t width, uint16_t height) {
    if (!m_has_client) {
        return false;
    }

    auto packet = PacketBuilder::build_video_frame(
        m_sequence_num++, frame_id, width, height, encoded_data);

    return send_to(m_client_addr, packet);
}

bool UDPTransport::send_connect_response(const sockaddr_in& client_addr, bool accepted,
                                          uint16_t width, uint16_t height, uint32_t bitrate_kbps) {
    auto packet = PacketBuilder::build_connect_response(
        m_sequence_num++, accepted, width, height, 0x01, bitrate_kbps);

    if (send_to(client_addr, packet)) {
        if (accepted) {
            m_client_addr = client_addr;
            m_has_client = true;
            spdlog::info("Client connected: {}:{}",
                        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }
        return true;
    }
    return false;
}

bool UDPTransport::broadcast_discovery(uint16_t port) {
    // Send broadcast discovery packet
    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr.sin_port = htons(port);

    auto packet = PacketBuilder::build_heartbeat(0);

    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<char*>(&broadcast), sizeof(broadcast));

    int result = sendto(sock, reinterpret_cast<const char*>(packet.data()),
                        static_cast<int>(packet.size()), 0,
                        reinterpret_cast<sockaddr*>(&broadcast_addr), sizeof(broadcast_addr));

    closesocket(sock);
    WSACleanup();

    return result != SOCKET_ERROR;
}

} // namespace penstream::network
