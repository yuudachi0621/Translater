#include "RateLimiter.h"

void RateLimiter::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastTime).count();

    // 计算最小间隔时间(微秒)
    int64_t min_interval = 1200000 / m_qps;

    if (elapsed < min_interval)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(min_interval - elapsed));
    }

    m_lastTime = std::chrono::steady_clock::now();
}
