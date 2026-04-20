#pragma once

#include "encoder_interface.h"
#include <windows.h>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>

#ifdef ENABLE_NVENC
// NVENC SDK includes - require NVIDIA Video Codec SDK
// Download from: https://developer.nvidia.com/nvidia-video-codec-sdk
// Add to path: $ENV{NVENC_SDK_PATH}/Interfaces
#include <nvEncodeAPI.h>
#endif

namespace penstream::encode {

class NVEncoder : public IEncoder {
public:
    NVEncoder();
    ~NVEncoder() override;

    bool initialize(const EncodeConfig& config) override;
    bool encode(const capture::Frame& frame, std::vector<uint8_t>& out_data) override;
    void shutdown() override;
    const char* name() const override { return "NVENC"; }

    // Set D3D11 device for interop (called after DXGI init)
    void set_d3d_device(ID3D11Device* device);

private:
#ifdef ENABLE_NVENC
    bool load_nvenc();
    bool create_encoder(const EncodeConfig& config);
    bool register_d3d11_resource();

    HMODULE m_nvenc_lib;
    NV_ENCODE_API_FUNCTION_LIST m_nvenc;
    void* m_encoder;
    ID3D11Device* m_d3d_device;
    ID3D11Texture2D* m_input_texture;
    void* m_registered_resource;

    // Encoder config
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_fps;
    uint32_t m_bitrate;
    bool m_low_latency;
    bool m_initialized;

    // Encode state
    uint32_t m_sequence_num;
    bool m_need_keyframe;
#endif
};

} // namespace penstream::encode
