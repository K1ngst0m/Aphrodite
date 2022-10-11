#ifndef TIMER_H_
#define TIMER_H_

#include "common.h"

namespace vkl {
class Timer {
public:
    Timer(float &interval)
        : _interval(interval) {
        start = std::chrono::steady_clock::now();
    }
    ~Timer() {
        using ms = std::chrono::duration<float, std::milli>;
        auto end  = std::chrono::steady_clock::now();
        _interval = std::chrono::duration_cast<ms>(start - end).count() / 1000.0f;
    }

private:
    std::chrono::steady_clock::time_point start;
    float                                &_interval;
};

} // namespace vkl

#endif // TIMER_H_
