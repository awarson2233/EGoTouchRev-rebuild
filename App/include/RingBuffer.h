#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>

namespace App {

// 简单的跨线程通信无锁（或基于锁）的环形缓冲区。
// 现阶段用 mutex+cv 即可满足 120Hz 以下的数据分发，后续有极大性能要求时可换做 true lock-free。
template<typename T, size_t Capacity>
class RingBuffer {
public:
    RingBuffer() : m_head(0), m_tail(0), m_count(0) {}

    bool Push(const T& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count >= Capacity) {
            // 缓冲区满，可选择丢弃旧帧或返回失败
            return false;
        }
        m_buffer[m_tail] = item;
        m_tail = (m_tail + 1) % Capacity;
        ++m_count;
        m_cv.notify_one();
        return true;
    }

    bool Pop(T& outItem) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count == 0) {
            return false;
        }
        outItem = m_buffer[m_head];
        m_head = (m_head + 1) % Capacity;
        --m_count;
        return true;
    }
    
    // 阻塞直到有新数据或超时 (用于处理线程)
    bool WaitForData(T& outItem, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_cv.wait_for(lock, timeout, [this] { return m_count > 0; })) {
            outItem = m_buffer[m_head];
            m_head = (m_head + 1) % Capacity;
            --m_count;
            return true;
        }
        return false;
    }
    
    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_tail = 0;
        m_count = 0;
    }

private:
    std::array<T, Capacity> m_buffer;
    size_t m_head;
    size_t m_tail;
    size_t m_count;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace App
