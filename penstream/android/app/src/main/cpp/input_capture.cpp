#include "input_capture.h"
#include <android/log.h>

#define LOG_TAG "PenStreamInput"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace penstream::android {

InputCapture::InputCapture()
    : m_callback(nullptr)
{}

InputCapture::~InputCapture() {}

void InputCapture::set_input_callback(void (*callback)(const InputData&)) {
    m_callback = callback;
}

void InputCapture::process_touch(float x, float y, float pressure) {
    if (m_callback == nullptr) {
        return;
    }

    InputData data;
    data.x = x;
    data.y = y;
    data.pressure = pressure;
    data.tilt_x = 0; // Would need hardware support
    data.tilt_y = 0;
    data.buttons = pressure > 0.01f ? 1 : 0;
    data.timestamp = 0; // Would use system time

    m_callback(data);
}

} // namespace penstream::android
