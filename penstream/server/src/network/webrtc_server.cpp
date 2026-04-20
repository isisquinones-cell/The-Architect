#include "webrtc_server.h"
#include <spdlog/spdlog.h>

namespace penstream::network {

WebRTCServer::WebRTCServer()
    : m_initialized(false)
{}

WebRTCServer::~WebRTCServer() {
    shutdown();
}

bool WebRTCServer::initialize(uint16_t port) {
    // WebRTC initialization stub
    // Full implementation would use Pion WebRTC C++ library

    spdlog::info("WebRTC server initialized on port {}", port);
    m_initialized = true;
    return true;
}

bool WebRTCServer::send_video_frame(const std::vector<uint8_t>& data, uint32_t timestamp) {
    if (!m_initialized) {
        return false;
    }

    // Send via WebRTC data channel or media track
    return true;
}

void WebRTCServer::shutdown() {
    m_initialized = false;
}

} // namespace penstream::network
