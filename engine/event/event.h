#ifndef APH_EVENT_H
#define APH_EVENT_H

#include "common/common.h"
#include "input/input.h"

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
    explicit Event(EventType type)
        : m_type(type)
    {
    }
    virtual ~Event() = default;

    EventType getType() const
    {
        return m_type;
    }

private:
    EventType m_type = EventType::UNDEFINED;
};

struct MouseButtonEvent : public Event
{
    MouseButtonEvent(MouseButton button, int absX, int absY, bool pressed)
        : Event(EventType::MOUSE_BTN)
        , m_button(button)
        , m_absX(absX)
        , m_absY(absY)
        , m_pressed(pressed)
    {
    }

    MouseButton m_button;
    int m_absX;
    int m_absY;
    bool m_pressed;
};

struct MouseMoveEvent : public Event
{
    MouseMoveEvent(int deltaX, int deltaY, int absX, int absY)
        : Event(EventType::MOUSE_MOVE)
        , m_deltaX(deltaX)
        , m_deltaY(deltaY)
        , m_absX(absX)
        , m_absY(absY)
    {
    }

    int m_deltaX;
    int m_deltaY;
    int m_absX;
    int m_absY;
};

struct KeyboardEvent : public Event
{
    explicit KeyboardEvent(Key key, KeyState state)
        : Event(EventType::KEY)
        , m_key(key)
        , m_state(state)
    {
    }

    Key m_key;
    KeyState m_state;
};

struct WindowResizeEvent : public Event
{
    explicit WindowResizeEvent(uint32_t width, uint32_t height)
        : Event(EventType::WINDOW_RESIZE)
        , m_width(width)
        , m_height(height)
    {
    }

    uint32_t m_width;
    uint32_t m_height;
};

} // namespace aph

#endif
