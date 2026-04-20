#include "virtual_input.h"
#include <spdlog/spdlog.h>

namespace penstream::input {

VirtualInput::VirtualInput()
    : m_screen_width(0)
    , m_screen_height(0)
    , m_initialized(false)
    , m_pen_down(false)
{
    // Get screen dimensions
    m_screen_width = GetSystemMetrics(SM_CXSCREEN);
    m_screen_height = GetSystemMetrics(SM_CYSCREEN);
}

VirtualInput::~VirtualInput() {
    shutdown();
}

bool VirtualInput::initialize() {
    // Check if we can send input
    m_initialized = true;
    spdlog::info("Virtual input initialized for {}x{}", m_screen_width, m_screen_height);
    return true;
}

void VirtualInput::send_input(const InputEvent& event) {
    if (!m_initialized) {
        return;
    }

    // Convert normalized coordinates to screen coordinates
    int x = static_cast<int>(event.x * m_screen_width);
    int y = static_cast<int>(event.y * m_screen_height);

    // Send mouse move
    send_mouse_move(x, y);

    // Handle pen down/up based on pressure
    bool is_pressed = event.pressure > 0.01f;

    if (is_pressed && !m_pen_down) {
        // Pen down
        send_mouse_down();
        m_pen_down = true;
    } else if (!is_pressed && m_pen_down) {
        // Pen up
        send_mouse_up();
        m_pen_down = false;
    }

    if (m_pen_down) {
        // Send pressure info (for apps that support it)
        send_pressure(event.pressure);
    }
}

void VirtualInput::send_mouse_move(int x, int y) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = x;
    input.mi.dy = y;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    SendInput(1, &input, sizeof(INPUT));
}

void VirtualInput::send_mouse_down() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    SendInput(1, &input, sizeof(INPUT));
}

void VirtualInput::send_mouse_up() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(1, &input, sizeof(INPUT));
}

void VirtualInput::send_pressure(float pressure) {
    // Windows doesn't have native pressure API for virtual devices
    // This would require a virtual tablet driver for full pressure support
    // For now, we just track it internally

    // Advanced: Could use Windows Ink API or create a virtual HID device
    // with ViGEmBus or similar
}

void VirtualInput::shutdown() {
    if (m_pen_down) {
        send_mouse_up();
        m_pen_down = false;
    }
    m_initialized = false;
}

} // namespace penstream::input
