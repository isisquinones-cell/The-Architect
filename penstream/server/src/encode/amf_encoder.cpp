#include "amf_encoder.h"
#include <spdlog/spdlog.h>

namespace penstream::encode {

AMFEncoder::AMFEncoder()
    : m_amf_lib(nullptr)
    , m_amf_factory(nullptr)
    , m_encoder(nullptr)
    , m_surface(nullptr)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
{}

AMFEncoder::~AMFEncoder() {
    shutdown();
}

bool AMFEncoder::initialize(const EncodeConfig& config) {
    if (!load_amf()) {
        spdlog::error("Failed to load AMF library");
        return false;
    }

    if (!create_encoder(config)) {
        spdlog::error("Failed to create AMF encoder");
        return false;
    }

    m_width = config.width;
    m_height = config.height;
    m_initialized = true;

    spdlog::info("AMF encoder initialized: {}x{}", config.width, config.height);
    return true;
}

bool AMFEncoder::load_amf() {
    m_amf_lib = LoadLibraryA("amf-vulkan.dll");
    if (!m_amf_lib) {
        m_amf_lib = LoadLibraryA("amf-dx11.dll");
    }

    if (!m_amf_lib) {
        spdlog::warn("Failed to load AMF DLLs");
        return false;
    }

    // AMF initialization would go here
    // This is a stub - full implementation requires AMF SDK
    return true;
}

bool AMFEncoder::create_encoder(const EncodeConfig& config) {
    // AMF encoder initialization
    // Full implementation requires AMF SDK integration
    return true;
}

bool AMFEncoder::encode(const capture::Frame& frame, std::vector<uint8_t>& out_data) {
    if (!m_initialized) {
        return false;
    }

    // Stub implementation
    // Full implementation would:
    // 1. Create AMF surface from frame data
    // 2. Submit to encoder
    // 3. Get encoded bitstream

    return false;
}

void AMFEncoder::shutdown() {
    if (m_encoder) {
        // Cleanup encoder
        m_encoder = nullptr;
    }

    if (m_amf_lib) {
        FreeLibrary(static_cast<HMODULE>(m_amf_lib));
        m_amf_lib = nullptr;
    }

    m_initialized = false;
}

} // namespace penstream::encode
