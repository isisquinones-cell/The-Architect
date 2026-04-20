#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <atomic>
#include "packet_builder.h"

namespace penstream::network {

struct NetworkStats {
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t packet_loss_percent; // Fixed point: 100 = 1%
    uint16_t latency_ms;
};

class UDPTransport {
public:
    explicit UDPTransport(uint16_t port);
    ~UDPTransport();

    bool initialize();
    void shutdown();

    bool send_to(const sockaddr_in& addr, const std::vector<uint8_t>& data);
    bool receive(std::vector<uint8_t>& out_data, sockaddr_in& out_addr, int timeout_ms = 10);

    bool send_video_frame(const std::vector<uint8_t>& encoded_data, uint32_t frame_id, uint64_t timestamp_us,
                          uint16_t width = 1920, uint16_t height = 1080);
    bool send_connect_response(const sockaddr_in& client_addr, bool accepted,
                               uint16_t width, uint16_t height, uint32_t bitrate_kbps);

    void set_client_address(const sockaddr_in& addr) { m_client_addr = addr; }
    bool has_client() const { return m_has_client; }
    NetworkStats get_stats() const { return m_stats; }

    // Discovery
    static bool broadcast_discovery(uint16_t port);

private:
    bool create_socket();
    bool bind_socket();

    SOCKET m_socket;
    sockaddr_in m_server_addr;
    sockaddr_in m_client_addr;
    uint16_t m_port;
    std::atomic<bool> m_has_client;

    NetworkStats m_stats;
    uint32_t m_sequence_num;
};

} // namespace penstream::network
