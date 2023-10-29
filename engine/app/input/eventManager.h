#ifndef EVENTMANAGER_H_
#define EVENTMANAGER_H_

#include <typeindex>
#include <any>
#include <mutex>
#include "threads/threadPool.h"

namespace aph
{

class EventManager
{
    template <typename TEvent>
    struct EventData
    {
        std::queue<TEvent>                              m_events;
        std::vector<std::function<bool(const TEvent&)>> m_handlers;

        void process()
        {
            auto& events   = m_events;
            auto& handlers = m_handlers;

            while(!events.empty())
            {
                auto e = events.front();
                events.pop();
                for(const auto& cb : handlers)
                {
                    cb(e);
                }
            }
        }
    };

public:
    EventManager(const EventManager&)            = delete;
    EventManager& operator=(const EventManager&) = delete;

    static EventManager& GetInstance()
    {
        static EventManager instance;
        return instance;
    }

    template <typename TEvent>
    void pushEvent(const TEvent& e)
    {
        std::lock_guard<std::mutex> lock(m_dataMapMutex);
        getEventData<TEvent>().m_events.push(e);
    }

    template <typename TEvent>
    void registerEventHandler(std::function<bool(const TEvent&)>&& func)
    {
        getEventData<TEvent>().m_handlers.push_back(std::move(func));
    }

    void processAll()
    {
        // TODO check that different event type don't cause data race
        std::vector<std::future<void>> results;
        results.reserve(eventDataMap.size());
        for(auto& keyValue : eventDataMap)
        {
            results.push_back(m_threadPools.enqueue([&keyValue]() { keyValue.second.second(keyValue.second.first); }));
        }
        for(auto& result : results)
        {
            result.wait();
        }
    }

    void processAllAsync()
    {
        // TODO check that different event type don't cause data race
        for(auto& keyValue : eventDataMap)
        {
            m_processingEvents.push_back(
                m_threadPools.enqueue([&keyValue]() { keyValue.second.second(keyValue.second.first); }));
        }
    }

    void flush()
    {
        for(auto& event : m_processingEvents)
        {
            event.wait();
        }
        m_processingEvents.clear();
    }

private:
    EventManager() = default;

    std::mutex                     m_dataMapMutex;
    ThreadPool<>                   m_threadPools;
    std::vector<std::future<void>> m_processingEvents;

    std::unordered_map<std::type_index, std::pair<std::any, std::function<void(std::any&)>>> eventDataMap;

    template <typename TEvent>
    EventData<TEvent>& getEventData()
    {
        auto ti = std::type_index(typeid(TEvent));
        if(!eventDataMap.contains(ti))
        {
            eventDataMap[ti] = {EventData<TEvent>{},
                                [](std::any& eventData) { std::any_cast<EventData<TEvent>&>(eventData).process(); }};
        }
        return std::any_cast<EventData<TEvent>&>(eventDataMap[ti].first);
    }
};

}  // namespace aph

#endif  // EVENTMANAGER_H_
