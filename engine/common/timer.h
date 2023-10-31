#ifndef TIMER_H_
#define TIMER_H_

#include <chrono>
#include <unordered_map>
#include <string>

#include "common/logger.h"
#include "common/singleton.h"

namespace aph
{

class Timer : public Singleton<Timer>
{
public:
    // Set a timestamp with a specific tag
    void set(const std::string& tag) { tags[tag] = Clock::now(); }

    // Calculate the interval between two timestamps using their tags
    double interval(std::string_view start, std::string_view end) const
    {
        auto it1 = tags.find(start.data());
        auto it2 = tags.find(end.data());

        if(it1 == tags.end() || it2 == tags.end())
        {
            CM_LOG_ERR("One or both tags not found!");
            return 0.0;
        }

        Duration duration = it2->second - it1->second;
        return duration.count();
    }

private:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration  = std::chrono::duration<double>;

    std::unordered_map<std::string, TimePoint> tags;
};

}  // namespace aph

#endif  // TIMER_H_
