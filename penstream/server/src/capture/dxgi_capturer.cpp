#include "dxgi_capturer.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace penstream::capture {

DXGICapturer::DXGICapturer()
    : m_device(nullptr)
    , m_context(nullptr)
    , m_duplication(nullptr)
    , m_staging_texture(nullptr)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
{}

DXGICapturer::~DXGICapturer() {
    shutdown();
}

bool DXGICapturer::initialize() {
    if (!create_device()) {
        return false;
    }

    if (!create_duplication()) {
        return false;
    }

    m_initialized = true;
    spdlog::info("DXGI capturer initialized: {}x{}", m_width, m_height);
    return true;
}

bool DXGICapturer::create_device() {
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        feature_levels,
        ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION,
        &m_device,
        nullptr,
        &m_context
    );

    if (FAILED(hr)) {
        spdlog::error("Failed to create D3D11 device: 0x{:08X}", hr);
        return false;
    }

    return true;
}

bool DXGICapturer::create_duplication() {
    IDXGIDevice* dxgi_device = nullptr;
    HRESULT hr = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));

    if (FAILED(hr)) {
        spdlog::error("Failed to get IDXGIDevice");
        return false;
    }

    IDXGIAdapter* adapter = nullptr;
    hr = dxgi_device->GetAdapter(&adapter);
    dxgi_device->Release();

    if (FAILED(hr)) {
        spdlog::error("Failed to get adapter");
        return false;
    }

    IDXGIOutput* output = nullptr;
    hr = adapter->EnumOutputs(0, &output);
    adapter->Release();

    if (FAILED(hr)) {
        spdlog::error("Failed to enum outputs");
        return false;
    }

    IDXGIOutput1* output1 = nullptr;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output1));
    output->Release();

    if (FAILED(hr)) {
        spdlog::error("Failed to get IDXGIOutput1");
        return false;
    }

    DXGI_OUTPUT_DESC desc;
    output1->GetDesc(&desc);
    m_width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
    m_height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

    hr = output1->DuplicateOutput(m_device, &m_duplication);
    output1->Release();

    if (FAILED(hr)) {
        spdlog::error("Failed to create duplication: 0x{:08X}", hr);
        return false;
    }

    // Crear staging texture para CPU access
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = m_width;
    staging_desc.Height = m_height;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    hr = m_device->CreateTexture2D(&staging_desc, nullptr, &m_staging_texture);
    if (FAILED(hr)) {
        spdlog::error("Failed to create staging texture: 0x{:08X}", hr);
        return false;
    }

    return true;
}

bool DXGICapturer::capture_frame(Frame& out_frame) {
    if (!m_initialized) {
        return false;
    }

    IDXGIResource* resource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frame_info;

    HRESULT hr = m_duplication->AcquireNextFrame(10, &frame_info, &resource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return false; // No frame available
    }

    if (FAILED(hr)) {
        spdlog::warn("AcquireNextFrame failed: 0x{:08X}", hr);
        return false;
    }

    // Copiar a staging texture
    ID3D11Texture2D* texture = nullptr;
    hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));
    resource->Release();

    if (FAILED(hr)) {
        return false;
    }

    m_context->CopyResource(m_staging_texture, texture);
    texture->Release();

    // Mapear staging texture
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = m_context->Map(m_staging_texture, 0, D3D11_MAP_READ, 0, &mapped);

    if (FAILED(hr)) {
        return false;
    }

    // Copiar datos
    out_frame.data.resize(m_width * m_height * 4);
    memcpy(out_frame.data.data(), mapped.pData, out_frame.data.size());

    m_context->Unmap(m_staging_texture, 0);

    out_frame.width = m_width;
    out_frame.height = m_height;
    out_frame.timestamp_us = frame_info.LastPresentTime.QuadPart;
    out_frame.is_keyframe = false; // Los keyframes los decide el encoder

    // Release frame
    m_duplication->ReleaseFrame();

    return true;
}

void DXGICapturer::shutdown() {
    if (m_duplication) {
        m_duplication->ReleaseFrame();
        m_duplication->Release();
        m_duplication = nullptr;
    }

    if (m_staging_texture) {
        m_staging_texture->Release();
        m_staging_texture = nullptr;
    }

    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }

    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }

    m_initialized = false;
}

} // namespace penstream::capture
