#include "input_handler.h"
#include <spdlog/spdlog.h>

namespace penstream::input {

InputHandler::InputHandler()
    : m_running(false)
{}

InputHandler::~InputHandler() {
    shutdown();
}

bool InputHandler::initialize() {
    if (!m_virtual_input.initialize()) {
        return false;
    }

    m_running = true;
    spdlog::info("Input handler initialized");
    return true;
}

void InputHandler::queue_input(const InputEvent& event) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_input_queue.push(event);
}

void InputHandler::process_pending_inputs() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);

    while (!m_input_queue.empty()) {
        InputEvent event = m_input_queue.front();
        m_input_queue.pop();
        lock.unlock();

        m_virtual_input.send_input(event);

        lock.lock();
    }
}

void InputHandler::shutdown() {
    m_running = false;
    m_virtual_input.shutdown();

    // Clear queue
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    while (!m_input_queue.empty()) {
        m_input_queue.pop();
    }

    spdlog::info("Input handler shutdown");
}

} // namespace penstream::input
