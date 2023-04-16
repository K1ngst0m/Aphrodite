#ifndef TIMER_H_
#define TIMER_H_

#include "common.h"

namespace aph
{
class Timer
{
public:
    Timer(float& interval) : m_interval(interval) { m_start = std::chrono::steady_clock::now(); }
    ~Timer()
    {
        using ms   = std::chrono::duration<float, std::milli>;
        auto end   = std::chrono::steady_clock::now();
        m_interval = std::chrono::duration_cast<ms>(m_start - end).count() / 1000.0f;
    }

private:
    std::chrono::steady_clock::time_point m_start;
    float&                                m_interval;
};

}  // namespace aph

#endif  // TIMER_H_
