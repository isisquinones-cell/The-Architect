#include "video_decoder.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG "PenStreamDecoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// AMEDIAFORMAT_MIME_TYPE_VIDEO_AVC fue introducido en API 29.
// Para API 26+ usamos el string literal directamente.
#ifndef AMEDIAFORMAT_MIME_TYPE_VIDEO_AVC
#define AMEDIAFORMAT_MIME_TYPE_VIDEO_AVC "video/avc"
#endif

// AMEDIACODEC_BUFFER_FLAG_KEY_FRAME no existe en NDK API 26.
// El valor equivalente es 1 (BUFFER_FLAG_SYNC_FRAME de la Java API).
#ifndef AMEDIACODEC_BUFFER_FLAG_KEY_FRAME
#define AMEDIACODEC_BUFFER_FLAG_KEY_FRAME 1
#endif

namespace penstream::android {

VideoDecoder::VideoDecoder()
    : m_codec(nullptr)
    , m_format(nullptr)
    , m_surface(nullptr)
    , m_width(1920)
    , m_height(1080)
    , m_initialized(false)
{}

VideoDecoder::~VideoDecoder() {
    release();
}

void VideoDecoder::set_surface(ANativeWindow* surface) {
    m_surface = surface;
    if (m_initialized && m_codec) {
        AMediaCodec_configure(m_codec, m_format, m_surface, nullptr, 0);
        LOGI("Decoder reconfigured with new surface");
    }
}

bool VideoDecoder::initialize(int32_t width, int32_t height, ANativeWindow* surface) {
    m_width  = width;
    m_height = height;
    m_surface = surface;

    m_format = AMediaFormat_new();
    AMediaFormat_setString(m_format, AMEDIAFORMAT_KEY_MIME, AMEDIAFORMAT_MIME_TYPE_VIDEO_AVC);
    AMediaFormat_setInt32(m_format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(m_format, AMEDIAFORMAT_KEY_HEIGHT, height);

    // AMEDIAFORMAT_KEY_LOW_LATENCY requiere API 30.
    // AMEDIAFORMAT_KEY_OPERATING_RATE requiere API 28.
    // Los omitimos para mantener compatibilidad con API 26.

    m_codec = AMediaCodec_createDecoderByType(AMEDIAFORMAT_MIME_TYPE_VIDEO_AVC);
    if (m_codec == nullptr) {
        LOGE("Failed to create decoder");
        return false;
    }

    media_status_t status = AMediaCodec_configure(m_codec, m_format, m_surface, nullptr, 0);
    if (status != AMEDIA_OK) {
        LOGE("Failed to configure decoder: %d", status);
        return false;
    }

    status = AMediaCodec_start(m_codec);
    if (status != AMEDIA_OK) {
        LOGE("Failed to start decoder: %d", status);
        return false;
    }

    m_initialized = true;
    LOGI("Decoder initialized: %dx%d", width, height);
    return true;
}

bool VideoDecoder::decode(const std::vector<uint8_t>& encoded_data, bool is_keyframe) {
    if (!m_initialized || !m_codec || encoded_data.empty()) {
        return false;
    }

    ssize_t index = AMediaCodec_dequeueInputBuffer(m_codec, 10000);
    if (index < 0) {
        LOGI("No input buffer available");
        return false;
    }

    size_t size;
    uint8_t* buffer = AMediaCodec_getInputBuffer(m_codec, static_cast<size_t>(index), &size);
    if (buffer == nullptr) {
        LOGE("Failed to get input buffer");
        return false;
    }

    size_t copy_size = std::min(size, encoded_data.size());
    memcpy(buffer, encoded_data.data(), copy_size);

    uint32_t flags = 0;
    if (is_keyframe) {
        flags = AMEDIACODEC_BUFFER_FLAG_KEY_FRAME;
        LOGI("Sending keyframe to decoder");
    }

    AMediaCodec_queueInputBuffer(m_codec, static_cast<size_t>(index), 0, copy_size, 0, flags);

    AMediaCodecBufferInfo info;
    ssize_t out_index;
    while ((out_index = AMediaCodec_dequeueOutputBuffer(m_codec, &info, 0)) >= 0) {
        AMediaCodec_releaseOutputBuffer(m_codec, static_cast<size_t>(out_index), true);
    }

    return true;
}

void VideoDecoder::release() {
    if (m_codec) {
        AMediaCodec_stop(m_codec);
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
    }
    if (m_format) {
        AMediaFormat_delete(m_format);
        m_format = nullptr;
    }
    m_initialized = false;
    LOGI("Decoder released");
}

} // namespace penstream::android
