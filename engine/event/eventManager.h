#pragma once

#include "common/hash.h"
#include "threads/taskManager.h"
#include <any>
#include <mutex>
#include <typeindex>

namespace aph
{

class EventManager
{
    template <typename TEvent>
    struct EventData
    {
        std::queue<TEvent> m_events;
        SmallVector<std::function<bool(const TEvent&)>> m_handlers;

        void process()
        {
            auto& events = m_events;
            auto& handlers = m_handlers;

            while (!events.empty())
            {
                auto e = events.front();
                events.pop();
                for (const auto& cb : handlers)
                {
                    cb(e);
                }
            }
        }
    };

public:
    EventManager()
    {
        m_pendingEvent = m_taskManager.createTaskGroup("event processing");
    }
    template <typename TEvent>
    void pushEvent(const TEvent& e)
    {
        std::lock_guard<std::mutex> lock(m_dataMapMutex);
        getEventData<TEvent>().m_events.push(e);
    }

    template <typename TEvent>
    void registerEvent(std::function<bool(const TEvent&)>&& func)
    {
        getEventData<TEvent>().m_handlers.push_back(std::move(func));
    }

    void processAll()
    {
        for (auto& [_, value] : m_eventDataMap)
        {
            value.second(value.first);
        }
    }

private:
    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;
    std::mutex m_dataMapMutex;

    HashMap<std::type_index, std::pair<std::any, std::function<void(std::any&)>>> m_eventDataMap;

    template <typename TEvent>
    EventData<TEvent>& getEventData()
    {
        auto ti = std::type_index(typeid(TEvent));
        if (!m_eventDataMap.contains(ti))
        {
            m_eventDataMap[ti] = { EventData<TEvent>{}, [](std::any& eventData)
                                   { std::any_cast<EventData<TEvent>&>(eventData).process(); } };
        }
        return std::any_cast<EventData<TEvent>&>(m_eventDataMap[ti].first);
    }

    TaskGroup* m_pendingEvent = {};
};

} // namespace aph
