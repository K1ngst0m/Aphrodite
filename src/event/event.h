#pragma once

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
    DPI_CHANGE,
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
    MouseButtonEvent(MouseButton button, float absX, float absY, bool pressed)
        : Event(EventType::MOUSE_BTN)
        , m_button(button)
        , m_absX(absX)
        , m_absY(absY)
        , m_pressed(pressed)
    {
    }

    MouseButton m_button;
    float m_absX;
    float m_absY;
    bool m_pressed;
};

struct MouseMoveEvent : public Event
{
    MouseMoveEvent(float deltaX, float deltaY, float absX, float absY)
        : Event(EventType::MOUSE_MOVE)
        , m_deltaX(deltaX)
        , m_deltaY(deltaY)
        , m_absX(absX)
        , m_absY(absY)
    {
    }

    float m_deltaX;
    float m_deltaY;
    float m_absX;
    float m_absY;
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

struct DPIChangeEvent : public Event
{
    explicit DPIChangeEvent(float dpiScale, uint32_t logicalWidth, uint32_t logicalHeight, uint32_t pixelWidth,
                            uint32_t pixelHeight)
        : Event(EventType::DPI_CHANGE)
        , m_dpiScale(dpiScale)
        , m_logicalWidth(logicalWidth)
        , m_logicalHeight(logicalHeight)
        , m_pixelWidth(pixelWidth)
        , m_pixelHeight(pixelHeight)
    {
    }

    float m_dpiScale;
    uint32_t m_logicalWidth;
    uint32_t m_logicalHeight;
    uint32_t m_pixelWidth;
    uint32_t m_pixelHeight;
};

} // namespace aph
