#pragma once

#include "common/hash.h"
#include "threads/taskManager.h"
#include <any>
#include <mutex>

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
            auto& events   = m_events;
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

    struct TypeErased
    {
        virtual ~TypeErased()  = default;
        virtual void process() = 0;
    };

    template <typename TEvent>
    struct TypedEventData : TypeErased
    {
        EventData<TEvent> data;

        void process() override
        {
            data.process();
        }
    };

public:
    EventManager()
    {
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
        for (auto& [_, handler] : m_eventDataMap)
        {
            handler->process();
        }
    }

private:
    std::mutex m_dataMapMutex;

    using TypeID = size_t;

    template <typename T>
    static TypeID getTypeID()
    {
        static TypeID id = nextTypeID();
        return id;
    }

    static TypeID nextTypeID()
    {
        static TypeID next = 0;
        return next++;
    }

    HashMap<TypeID, std::unique_ptr<TypeErased>> m_eventDataMap;

    template <typename TEvent>
    EventData<TEvent>& getEventData()
    {
        auto typeID = getTypeID<TEvent>();
        auto it     = m_eventDataMap.find(typeID);

        if (it == m_eventDataMap.end())
        {
            auto typedData         = std::make_unique<TypedEventData<TEvent>>();
            auto& result           = typedData->data;
            m_eventDataMap[typeID] = std::move(typedData);
            return result;
        }

        return static_cast<TypedEventData<TEvent>*>(it->second.get())->data;
    }
};

} // namespace aph
