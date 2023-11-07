#ifndef EVENTMANAGER_H_
#define EVENTMANAGER_H_

#include <typeindex>
#include <any>
#include <mutex>
#include "threads/taskManager.h"

namespace aph
{

class EventManager : public Singleton<EventManager>
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
        processAllAsync();
        flush();
    }

    void processAllAsync()
    {
        auto  group       = m_taskManager.createTaskGroup("event processing");
        // TODO check that different event type don't cause data race
        for(auto& [_, value] : m_eventDataMap)
        {
            group->addTask([&value]() { value.second(value.first); });
        }
        m_taskManager.submit(group);
    }

    void flush() { m_taskManager.wait(); }

private:
    TaskManager m_taskManager = {5, "Event Manager"};
    std::mutex m_dataMapMutex;

    std::unordered_map<std::type_index, std::pair<std::any, std::function<void(std::any&)>>> m_eventDataMap;

    template <typename TEvent>
    EventData<TEvent>& getEventData()
    {
        auto ti = std::type_index(typeid(TEvent));
        if(!m_eventDataMap.contains(ti))
        {
            m_eventDataMap[ti] = {EventData<TEvent>{},
                                  [](std::any& eventData) { std::any_cast<EventData<TEvent>&>(eventData).process(); }};
        }
        return std::any_cast<EventData<TEvent>&>(m_eventDataMap[ti].first);
    }
};

}  // namespace aph

#endif  // EVENTMANAGER_H_
