#ifndef TIMER_H_
#define TIMER_H_

#include "common/hash.h"
#include "common/logger.h"
#include "common/singleton.h"

namespace aph
{

class Timer : public Singleton<Timer>
{
public:
    // Set a timestamp with a specific tag
    void set(const std::string& tag) { m_strMap[tag] = Clock::now(); }
    void set(uint32_t tag) { m_numMap[tag] = Clock::now(); }

    // Calculate the interval between two timestamps using their tags
    double interval(std::string_view start, std::string_view end) const
    {
        auto it1 = m_strMap.find(start.data());
        auto it2 = m_strMap.find(end.data());

        if(it1 == m_strMap.end() || it2 == m_strMap.end())
        {
            CM_LOG_ERR("One or both tags not found!");
            return 0.0;
        }

        Duration duration = it2->second - it1->second;
        return duration.count();
    }

    // Calculate the interval between two timestamps using their tags
    double interval(uint32_t start, uint32_t end)
    {
        auto it1 = m_numMap.find(start);
        auto it2 = m_numMap.find(end);

        if(it1 == m_numMap.end() || it2 == m_numMap.end())
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

    HashMap<std::string, TimePoint> m_strMap;
    HashMap<uint32_t, TimePoint>    m_numMap;
};

}  // namespace aph

#endif  // TIMER_H_
