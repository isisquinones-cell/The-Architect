#include "nvenc_encoder.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace penstream::encode {

#ifdef ENABLE_NVENC

NVEncoder::NVEncoder()
    : m_nvenc_lib(nullptr)
    , m_encoder(nullptr)
    , m_d3d_device(nullptr)
    , m_input_texture(nullptr)
    , m_registered_resource(nullptr)
    , m_width(0)
    , m_height(0)
    , m_fps(0)
    , m_bitrate(0)
    , m_low_latency(false)
    , m_initialized(false)
    , m_sequence_num(0)
    , m_need_keyframe(true)
{
    memset(&m_nvenc, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
}

NVEncoder::~NVEncoder() {
    shutdown();
}

void NVEncoder::set_d3d_device(ID3D11Device* device) {
    m_d3d_device = device;
    spdlog::info("D3D11 device set for NVENC interop");
}

bool NVEncoder::initialize(const EncodeConfig& config) {
    if (!load_nvenc()) {
        spdlog::error("Failed to load NVENC library");
        return false;
    }

    uint32_t version = 0;
    NVENCSTATUS status = m_nvenc.nvEncodeAPIGetMaxSupportedVersion(&version);
    if (status != NV_ENC_SUCCESS) {
        spdlog::error("Failed to get NVENC version");
        return false;
    }
    spdlog::info("NVENC API version: {}.{}", (version >> 4) & 0xFF, version & 0xF);

    if (!create_encoder(config)) {
        spdlog::error("Failed to create NVENC encoder");
        return false;
    }

    m_width = config.width;
    m_height = config.height;
    m_fps = config.fps;
    m_bitrate = config.bitrate_bps;
    m_low_latency = config.low_latency;
    m_initialized = true;

    spdlog::info("NVENC encoder initialized: {}x{}@{}fps, {}kbps, low_latency={}",
                 config.width, config.height, config.fps,
                 config.bitrate_bps / 1000, config.low_latency);
    return true;
}

bool NVEncoder::load_nvenc() {
    m_nvenc_lib = LoadLibraryA("nvEncodeAPI64.dll");
    if (!m_nvenc_lib) {
        spdlog::error("Failed to load nvEncodeAPI64.dll - NVIDIA driver not installed?");
        return false;
    }

    auto create_instance = reinterpret_cast<NvEncodeAPICreateInstance_t>(
        GetProcAddress(m_nvenc_lib, "NvEncodeAPICreateInstance"));

    if (!create_instance) {
        spdlog::error("Failed to get NvEncodeAPICreateInstance function");
        FreeLibrary(m_nvenc_lib);
        m_nvenc_lib = nullptr;
        return false;
    }

    m_nvenc.version = NV_ENCODE_API_FUNCTION_LIST_VER;
    NVENCSTATUS status = create_instance(&m_nvenc);
    if (status != NV_ENC_SUCCESS) {
        spdlog::error("NvEncodeAPICreateInstance failed: {}", status);
        FreeLibrary(m_nvenc_lib);
        m_nvenc_lib = nullptr;
        return false;
    }

    spdlog::info("NVENC API loaded successfully");
    return true;
}

bool NVEncoder::create_encoder(const EncodeConfig& config) {
    if (!m_d3d_device) {
        spdlog::warn("D3D11 device not set yet - encoder will be created when device is available");
        return true;
    }

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS session_params = {};
    session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    session_params.device = m_d3d_device;
    session_params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
    session_params.apiVersion = NVENCAPI_VERSION;

    void* encoder = nullptr;
    NVENCSTATUS status = m_nvenc.nvEncOpenEncodeSessionEx(&session_params, &encoder);
    if (status != NV_ENC_SUCCESS) {
        spdlog::error("nvEncOpenEncodeSessionEx failed: {}", status);
        return false;
    }
    m_encoder = encoder;

    NV_ENC_PRESET_CONFIG preset_config = {};
    preset_config.version = NV_ENC_PRESET_CONFIG_VER;
    preset_config.presetCfg.version = NV_ENC_CONFIG_VER;

    status = m_nvenc.nvEncGetEncodePresetConfig(
        m_encoder,
        NV_ENC_CODEC_H264_GUID,
        NV_ENC_PRESET_P1_GUID,
        &preset_config
    );
    if (status != NV_ENC_SUCCESS) {
        spdlog::warn("Failed to get preset config, using defaults");
    }

    NV_ENC_INITIALIZE_PARAMS init_params = {};
    init_params.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init_params.encodeGUID = NV_ENC_CODEC_H264_GUID;
    init_params.presetGUID = NV_ENC_PRESET_P1_GUID;
    init_params.encodeWidth = config.width;
    init_params.encodeHeight = config.height;
    init_params.darWidth = config.width;
    init_params.darHeight = config.height;
    init_params.frameRateNum = config.fps;
    init_params.frameRateDen = 1;
    init_params.enablePTD = 1;
    init_params.enableEncodeAsync = 0;
    init_params.tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
    init_params.presetConfigPtr = &preset_config;

    NV_ENC_CONFIG* encode_config = &init_params.encodeConfig;
    encode_config->version = NV_ENC_CONFIG_VER;
    encode_config->gopLength = NV_ENC_INFINITE_GOPLENGTH;
    encode_config->frameIntervalP = 1;
    encode_config->monoChromeEncoding = 0;
    encode_config->rcParams.version = NV_ENC_RC_PARAMS_VER;
    encode_config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
    encode_config->rcParams.zeroReorderDelay = 1;
    encode_config->rcParams.enableLookahead = 0;
    encode_config->rcParams.averageBitRate = config.bitrate_bps;
    encode_config->rcParams.maxBitRate = config.bitrate_bps * 12 / 10;
    encode_config->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;

    status = m_nvenc.nvEncInitializeEncoder(m_encoder, &init_params);
    if (status != NV_ENC_SUCCESS) {
        spdlog::error("nvEncInitializeEncoder failed: {}", status);
        return false;
    }

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = config.width;
    tex_desc.Height = config.height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_NV12;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    tex_desc.CPUAccessFlags = 0;

    HRESULT hr = m_d3d_device->CreateTexture2D(&tex_desc, nullptr, &m_input_texture);
    if (FAILED(hr)) {
        spdlog::error("Failed to create input texture: 0x{:08X}", hr);
        return false;
    }

    NV_ENC_REGISTER_RESOURCE register_res = {};
    register_res.version = NV_ENC_REGISTER_RESOURCE_VER;
    register_res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
    register_res.resourceToRegister = m_input_texture;
    register_res.width = config.width;
    register_res.height = config.height;
    register_res.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

    status = m_nvenc.nvEncRegisterResource(m_encoder, &register_res);
    if (status != NV_ENC_SUCCESS) {
        spdlog::error("nvEncRegisterResource failed: {}", status);
        return false;
    }
    m_registered_resource = register_res.registeredResource;

    spdlog::info("NVENC encoder created with D3D11 interop");
    return true;
}

bool NVEncoder::encode(const capture::Frame& frame, std::vector<uint8_t>& out_data) {
    if (!m_initialized || !m_encoder) {
        return false;
    }

    if (!m_input_texture && m_d3d_device) {
        if (!create_encoder({
            .width = m_width,
            .height = m_height,
            .bitrate_bps = m_bitrate,
            .fps = m_fps,
            .low_latency = m_low_latency
        })) {
            return false;
        }
    }

    if (m_input_texture && !frame.data.empty()) {
        ID3D11DeviceContext* context = nullptr;
        m_d3d_device->GetImmediateContext(&context);
        if (context) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT hr = context->Map(m_input_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (SUCCEEDED(hr)) {
                const uint8_t* src = frame.data.data();
                uint8_t* dst = static_cast<uint8_t*>(mapped.pData);
                size_t copy_size = std::min(static_cast<size_t>(mapped.RowPitch * m_height),
                                           frame.data.size());
                memcpy(dst, src, copy_size);
                context->Unmap(m_input_texture, 0);
            }
            context->Release();
        }
    }

    NV_ENC_PIC_PARAMS pic_params = {};
    pic_params.version = NV_ENC_PIC_PARAMS_VER;
    pic_params.inputBuffer = m_registered_resource;
    pic_params.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
    pic_params.inputWidth = m_width;
    pic_params.inputHeight = m_height;
    pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    pic_params.encodePicFlags = 0;

    if (m_need_keyframe) {
        pic_params.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
        m_need_keyframe = false;
    }

    NV_ENC_LOCK_BITSTREAM lock_params = {};
    lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
    lock_params.doNotWait = 0;
    lock_params.outputBitstream = m_nvenc.nvEncCreateBitstreamBuffer(m_encoder, nullptr);

    NVENCSTATUS status = m_nvenc.nvEncEncodePicture(m_encoder, &pic_params);
    if (status != NV_ENC_SUCCESS) {
        spdlog::warn("nvEncEncodePicture failed: {}", status);
        return false;
    }

    status = m_nvenc.nvEncLockBitstream(m_encoder, &lock_params);
    if (status == NV_ENC_SUCCESS) {
        out_data.resize(lock_params.bitstreamSizeInBytes);
        memcpy(out_data.data(), lock_params.bitstreamBufferPtr, lock_params.bitstreamSizeInBytes);
        m_nvenc.nvEncUnlockBitstream(m_encoder, lock_params.outputBitstream);
        m_sequence_num++;
        return true;
    }

    return false;
}

void NVEncoder::shutdown() {
    if (m_encoder && m_nvenc.nvEncDestroyEncoder) {
        m_nvenc.nvEncDestroyEncoder(m_encoder);
        m_encoder = nullptr;
    }
    if (m_registered_resource && m_nvenc.nvEncUnregisterResource) {
        m_nvenc.nvEncUnregisterResource(m_registered_resource);
        m_registered_resource = nullptr;
    }
    if (m_input_texture) {
        m_input_texture->Release();
        m_input_texture = nullptr;
    }
    if (m_nvenc_lib) {
        FreeLibrary(m_nvenc_lib);
        m_nvenc_lib = nullptr;
    }
    m_initialized = false;
    spdlog::info("NVENC encoder shutdown complete");
}

#else // ENABLE_NVENC not defined — stub implementation

NVEncoder::NVEncoder() {}
NVEncoder::~NVEncoder() {}
void NVEncoder::set_d3d_device(ID3D11Device*) {}

bool NVEncoder::initialize(const EncodeConfig&) {
    spdlog::warn("NVENC support not compiled in (ENABLE_NVENC not defined). "
                 "Build with NVENC SDK to enable hardware encoding.");
    return false;
}

bool NVEncoder::encode(const capture::Frame&, std::vector<uint8_t>&) {
    return false;
}

void NVEncoder::shutdown() {}

#endif // ENABLE_NVENC

} // namespace penstream::encode
