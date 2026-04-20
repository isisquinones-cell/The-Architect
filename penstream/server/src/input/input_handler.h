#pragma once

#include "virtual_input.h"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

namespace penstream::input {

class InputHandler {
public:
    InputHandler();
    ~InputHandler();

    bool initialize();
    void queue_input(const InputEvent& event);
    void process_pending_inputs();
    void shutdown();

private:
    std::queue<InputEvent> m_input_queue;
    std::mutex m_queue_mutex;
    VirtualInput m_virtual_input;
    std::atomic<bool> m_running;
};

} // namespace penstream::input
