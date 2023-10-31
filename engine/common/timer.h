#ifndef TIMER_H_
#define TIMER_H_

#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>

#include "common/singleton.h"

namespace aph
{

class Timer : public Singleton<Timer>
{
public:
    // Set a timestamp with a specific tag
    void set(const std::string& tag) { tags[tag] = Clock::now(); }

    // Calculate the interval between two timestamps using their tags
    double interval(const std::string& tag1, const std::string& tag2) const
    {
        auto it1 = tags.find(tag1);
        auto it2 = tags.find(tag2);

        if(it1 == tags.end() || it2 == tags.end())
        {
            std::cerr << "One or both tags not found!" << '\n';
            return 0.0;
        }

        Duration duration = it2->second - it1->second;
        return duration.count();  // returns the interval in seconds
    }

private:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration  = std::chrono::duration<double>;

    std::unordered_map<std::string, TimePoint> tags;
};

}  // namespace aph

#endif  // TIMER_H_
