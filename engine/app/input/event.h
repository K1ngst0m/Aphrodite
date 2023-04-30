#ifndef APH_EVENT_H
#define APH_EVENT_H

#include "common/common.h"
#include "input.h"

namespace aph
{

enum class EventType
{
    UNDEFINED,
    KEY,
    MOUSE_MOVE,
    MOUSE_BTN,
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

class EventManager
{
    // TODO
};

}  // namespace aph

#endif
