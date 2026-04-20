#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <vector>
#include <chrono>

namespace penstream::capture {

struct Frame {
    std::vector<uint8_t> data;
    uint32_t width;
    uint32_t height;
    uint64_t timestamp_us;
    bool is_keyframe;
};

class DXGICapturer {
public:
    DXGICapturer();
    ~DXGICapturer();

    bool initialize();
    bool capture_frame(Frame& out_frame);
    void shutdown();

    uint32_t get_width() const { return m_width; }
    uint32_t get_height() const { return m_height; }
    ID3D11Device* get_device() const { return m_device; }

private:
    bool create_device();
    bool create_duplication();

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGIOutputDuplication* m_duplication;
    ID3D11Texture2D* m_staging_texture;

    uint32_t m_width;
    uint32_t m_height;
    bool m_initialized;
};

} // namespace penstream::capture
