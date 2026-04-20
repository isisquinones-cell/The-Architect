#pragma once

#include <cstdint>

namespace penstream::android {

struct InputData {
    float x;
    float y;
    float pressure;
    int8_t tilt_x;
    int8_t tilt_y;
    uint8_t buttons;
    uint64_t timestamp;
};

class InputCapture {
public:
    InputCapture();
    ~InputCapture();

    void set_input_callback(void (*callback)(const InputData&));
    void process_touch(float x, float y, float pressure);

private:
    void (*m_callback)(const InputData&);
};

} // namespace penstream::android
