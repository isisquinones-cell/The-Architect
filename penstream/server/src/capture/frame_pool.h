#pragma once

#include "dxgi_capturer.h"
#include <vector>
#include <queue>
#include <memory>
#include <mutex>

namespace penstream::capture {

class FramePool {
public:
    explicit FramePool(size_t pool_size = 3);
    ~FramePool();

    Frame* acquire();
    void release(Frame* frame);
    void clear();

private:
    size_t m_pool_size;
    std::vector<std::unique_ptr<Frame>> m_frames;
    std::vector<Frame*> m_available;
    std::mutex m_mutex;
};

} // namespace penstream::capture
