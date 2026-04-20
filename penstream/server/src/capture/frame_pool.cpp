#include "frame_pool.h"
#include <spdlog/spdlog.h>

namespace penstream::capture {

FramePool::FramePool(size_t pool_size)
    : m_pool_size(pool_size)
{}

FramePool::~FramePool() {
    clear();
}

Frame* FramePool::acquire() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_available.empty()) {
        // Crear nuevo frame si no hay disponibles
        auto frame = std::make_unique<Frame>();
        frame->data.reserve(1920 * 1080 * 4); // 1080p buffer
        m_frames.push_back(std::move(frame));
        return m_frames.back().get();
    }

    Frame* frame = m_available.front();
    m_available.erase(m_available.begin());
    return frame;
}

void FramePool::release(Frame* frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_available.push_back(frame);
}

void FramePool::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames.clear();
    m_available.clear();
}

} // namespace penstream::capture
