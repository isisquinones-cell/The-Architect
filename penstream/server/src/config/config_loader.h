#pragma once

#include <cstdint>
#include <string>

namespace penstream::config {

struct Config {
    // Network
    uint16_t port = 9696;

    // Video
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    uint32_t bitrate_kbps = 10000;

    // Encoder
    std::string encoder = "nvenc"; // nvenc, amf, qsv
    bool low_latency = true;

    // Security
    bool require_pin = false;
    std::string pin;

    // Advanced
    uint32_t fec_strength = 1; // Number of FEC packets per N data packets
    bool enable_stats = true;
};

Config load_config();
bool save_config(const Config& config);

} // namespace penstream::config
