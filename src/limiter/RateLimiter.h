#pragma once
#include <chrono>
#include <mutex>
class RateLimiter
{
public:
    RateLimiter(int qps)
        : m_qps(qps > 0 ? qps : 1), m_lastTime(std::chrono::steady_clock::now()) {}

    void wait();

private:
    int m_qps;
    std::chrono::steady_clock::time_point m_lastTime;
    std::mutex m_mutex;
};
