#pragma once

#include <cstdint>
#include <vector>
#include <atomic>

namespace penstream::network {

class WebRTCServer {
public:
    WebRTCServer();
    ~WebRTCServer();

    bool initialize(uint16_t port);
    bool send_video_frame(const std::vector<uint8_t>& data, uint32_t timestamp);
    void shutdown();

    bool is_initialized() const { return m_initialized; }

private:
    std::atomic<bool> m_initialized;
};

} // namespace penstream::network
