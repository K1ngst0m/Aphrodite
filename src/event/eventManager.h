#pragma once

#include "common/hash.h"
#include "threads/taskManager.h"
#include <mutex>

namespace aph
{

class EventManager
{
    template <typename TEvent>
    struct EventData
    {
        EventData() = default;
        EventData(const EventData&)                    = delete;
        EventData(EventData&&)                 = default;
        auto operator=(const EventData&) -> EventData& = delete;
        auto operator=(EventData&&) -> EventData&      = default;

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
    EventManager() = default;
    template <typename TEvent>
    void pushEvent(const TEvent& e);

    template <typename TEvent>
    void registerEvent(std::function<bool(const TEvent&)>&& func);

    void processAll();

private:
    std::mutex m_dataMapMutex;

    using TypeID = size_t;

    template <typename T>
    static auto getTypeID() -> TypeID;

    static auto nextTypeID() -> TypeID;

    HashMap<TypeID, std::unique_ptr<TypeErased>> m_eventDataMap;

    template <typename TEvent>
    auto getEventData() -> EventData<TEvent>&;
};

template <typename TEvent>
inline auto EventManager::getEventData() -> EventData<TEvent>&
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

template <typename T>
inline auto EventManager::getTypeID() -> TypeID
{
    static TypeID id = nextTypeID();
    return id;
}

template <typename TEvent>
inline void EventManager::registerEvent(std::function<bool(const TEvent&)>&& func)
{
    getEventData<TEvent>().m_handlers.push_back(std::move(func));
}

template <typename TEvent>
inline void EventManager::pushEvent(const TEvent& e)
{
    std::lock_guard<std::mutex> lock(m_dataMapMutex);
    getEventData<TEvent>().m_events.push(e);
}

} // namespace aph
