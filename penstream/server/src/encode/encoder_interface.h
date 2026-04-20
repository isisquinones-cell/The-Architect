#pragma once

#include "capture/dxgi_capturer.h"
#include <vector>
#include <cstdint>

namespace penstream::encode {

struct EncodeConfig {
    uint32_t width;
    uint32_t height;
    uint32_t bitrate_bps;
    uint32_t fps;
    bool low_latency;
};

class IEncoder {
public:
    virtual ~IEncoder() = default;
    virtual bool initialize(const EncodeConfig& config) = 0;
    virtual bool encode(const capture::Frame& frame, std::vector<uint8_t>& out_data) = 0;
    virtual void shutdown() = 0;
    virtual const char* name() const = 0;
};

} // namespace penstream::encode
