#ifndef APH_EVENT_H
#define APH_EVENT_H

#include "common/common.h"
#include "input.h"

#include <typeindex>
#include <any>
#include <mutex>

namespace aph
{

enum class EventType
{
    UNDEFINED,
    KEY,
    MOUSE_MOVE,
    MOUSE_BTN,
    WINDOW_RESIZE,
};

class Event
{
public:
    explicit Event(EventType type) : m_type(type) {}
    virtual ~Event() = default;

    EventType getType() const { return m_type; }

private:
    EventType m_type = EventType::UNDEFINED;
};

struct MouseButtonEvent : public Event
{
    MouseButtonEvent(MouseButton button, double absX, double absY, bool pressed) :
        Event(EventType::MOUSE_BTN),
        m_button(button),
        m_absX(absX),
        m_absY(absY),
        m_pressed(pressed)
    {
    }

    MouseButton m_button;
    double      m_absX;
    double      m_absY;
    bool        m_pressed;
};

struct MouseMoveEvent : public Event
{
    MouseMoveEvent(double deltaX, double deltaY, double absX, double absY) :
        Event(EventType::MOUSE_MOVE),
        m_deltaX(deltaX),
        m_deltaY(deltaY),
        m_absX(absX),
        m_absY(absY)
    {
    }

    double m_deltaX;
    double m_deltaY;
    double m_absX;
    double m_absY;
};

struct KeyboardEvent : public Event
{
    explicit KeyboardEvent(Key key, KeyState state) : Event(EventType::KEY), m_key(key), m_state(state) {}

    Key      m_key;
    KeyState m_state;
};

struct WindowResizeEvent : public Event
{
    explicit WindowResizeEvent(uint32_t width, uint32_t height) :
        Event(EventType::WINDOW_RESIZE),
        m_width(width),
        m_height(height)
    {
    }

    uint32_t m_width;
    uint32_t m_height;
};

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

class EventManager
{
public:
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    static EventManager& GetInstance()
    {
        static EventManager instance;
        return instance;
    }

    template <typename TEvent>
    void pushEvent(const TEvent& e)
    {
        getEventData<TEvent>().m_events.push(e);
    }

    template <typename TEvent>
    void registerEventHandler(std::function<bool(const TEvent&)>&& func)
    {
        getEventData<TEvent>().m_handlers.push_back(std::move(func));
    }

    void processAll()
    {
        for(auto& [ti, pair] : eventDataMap)
        {
            pair.second(pair.first);
        }
    }

private:
    EventManager() = default;

    std::mutex m_dataMapMutex;

    std::unordered_map<std::type_index, std::pair<std::any, std::function<void(std::any&)>>> eventDataMap;

    template <typename TEvent>
    EventData<TEvent>& getEventData()
    {
        std::lock_guard<std::mutex> lock(m_dataMapMutex);
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

#endif
