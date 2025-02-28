#ifndef TIMER_H_
#define TIMER_H_

#include "common/common.h"
#include "common/hash.h"
#include "common/logger.h"
#include "common/singleton.h"

namespace aph
{

enum class TimeUnit
{
    Seconds,
    MillSeconds,
    MicroSeconds,
    NanoSeconds
};

class Timer
{
public:
    // Set a timestamp with a specific tag
    APH_ALWAYS_INLINE void set(const std::string& tag)
    {
        std::lock_guard<std::mutex> m_holder{ m_lock };
        m_strMap[tag] = Clock::now();
    }
    APH_ALWAYS_INLINE void set(uint32_t tag)
    {
        std::lock_guard<std::mutex> m_holder{ m_lock };
        m_numMap[tag] = Clock::now();
    }

    // Calculate the elapsed time since setting one tag
    APH_ALWAYS_INLINE double interval(std::string_view tag) const
    {
        auto it1 = m_strMap.find(tag.data());

        if (it1 == m_strMap.end())
        {
            CM_LOG_ERR("Tag %s not found!", tag.data());
            return 0.0;
        }

        auto current = Clock::now();
        Duration duration = current - it1->second;
        return duration.count();
    }

    // Calculate the elapsed time since setting one tag
    APH_ALWAYS_INLINE double interval(uint32_t tag) const
    {
        auto it1 = m_numMap.find(tag);

        if (it1 == m_numMap.end())
        {
            CM_LOG_ERR("Tag %u not found!", tag);
            return 0.0;
        }

        auto current = Clock::now();
        Duration duration = current - it1->second;
        return duration.count();
    }

    // Calculate the interval between two timestamps using their tags
    APH_ALWAYS_INLINE double interval(std::string_view start, std::string_view end) const
    {
        auto it1 = m_strMap.find(start.data());
        auto it2 = m_strMap.find(end.data());

        if (it1 == m_strMap.end() || it2 == m_strMap.end())
        {
            CM_LOG_ERR("One or both tags not found!");
            return 0.0;
        }

        Duration duration = it2->second - it1->second;
        return duration.count();
    }

    // Calculate the interval between two timestamps using their tags
    APH_ALWAYS_INLINE double interval(uint32_t start, uint32_t end)
    {
        auto it1 = m_numMap.find(start);
        auto it2 = m_numMap.find(end);

        if (it1 == m_numMap.end() || it2 == m_numMap.end())
        {
            CM_LOG_ERR("One or both tags not found!");
            return 0.0;
        }

        Duration duration = it2->second - it1->second;

        return duration.count();
    }

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;

    HashMap<std::string, TimePoint> m_strMap;
    HashMap<uint32_t, TimePoint> m_numMap;
    std::mutex m_lock;
};

} // namespace aph

#endif // TIMER_H_
