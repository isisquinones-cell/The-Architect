#pragma once

#include <vector>
#include <cstdint>
#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

namespace penstream::android {

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    bool initialize(int32_t width, int32_t height, ANativeWindow* surface);
    bool decode(const std::vector<uint8_t>& encoded_data, bool is_keyframe = false);
    void release();

    int32_t get_output_width() const { return m_width; }
    int32_t get_output_height() const { return m_height; }

    void set_surface(ANativeWindow* surface);

private:
    AMediaCodec* m_codec;
    AMediaFormat* m_format;
    ANativeWindow* m_surface;
    int32_t m_width;
    int32_t m_height;
    bool m_initialized;
};

} // namespace penstream::android
