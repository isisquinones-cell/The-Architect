#pragma once

#include <windows.h>
#include <cstdint>

namespace penstream::input {

struct InputEvent {
    float x;            // Normalized X (0.0 - 1.0)
    float y;            // Normalized Y (0.0 - 1.0)
    float pressure;     // 0.0 - 1.0
    int8_t tilt_x;      // -90 to +90 degrees
    int8_t tilt_y;      // -90 to +90 degrees
    uint8_t buttons;    // Bitmask
    uint64_t timestamp;
};

class VirtualInput {
public:
    VirtualInput();
    ~VirtualInput();

    bool initialize();
    void send_input(const InputEvent& event);
    void shutdown();

private:
    void send_mouse_move(int x, int y);
    void send_mouse_down();
    void send_mouse_up();
    void send_pressure(float pressure);

    int m_screen_width;
    int m_screen_height;
    bool m_initialized;
    bool m_pen_down;
};

} // namespace penstream::input
