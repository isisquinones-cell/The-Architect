#pragma once

#include "encoder_interface.h"
#include <windows.h>
#include <vector>

namespace penstream::encode {

class AMFEncoder : public IEncoder {
public:
    AMFEncoder();
    ~AMFEncoder() override;

    bool initialize(const EncodeConfig& config) override;
    bool encode(const capture::Frame& frame, std::vector<uint8_t>& out_data) override;
    void shutdown() override;
    const char* name() const override { return "AMF (AMD)"; }

private:
    bool load_amf();
    bool create_encoder(const EncodeConfig& config);

    void* m_amf_lib;
    void* m_amf_factory;
    void* m_encoder;
    void* m_surface;

    uint32_t m_width;
    uint32_t m_height;
    bool m_initialized;
};

} // namespace penstream::encode
