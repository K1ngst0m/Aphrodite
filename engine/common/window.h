#ifndef WINDOW_H_
#define WINDOW_H_

#include "app/input/event.h"
#include "common.h"

class GLFWwindow;

namespace aph
{

class Window
{
private:
    Window(uint32_t width, uint32_t height);

public:
    static std::shared_ptr<Window> Create(uint32_t width = 800, uint32_t height = 600);
    ~Window();

public:
    float       getAspectRatio() const { return static_cast<float>(m_width) / m_height; }
    uint32_t    getWidth() const { return m_width; }
    uint32_t    getHeight() const { return m_height; }
    GLFWwindow* getHandle() { return m_window; }

    template <typename TEvent>
    void pushEvent(const TEvent& e)
    {
        if constexpr(std::is_same_v<TEvent, KeyboardEvent>)
        {
            m_keyboardsEvent.m_events.push(e);
        }
        else if constexpr(std::is_same_v<TEvent, MouseMoveEvent>)
        {
            m_mouseMoveEvent.m_events.push(e);
        }
        else if constexpr(std::is_same_v<TEvent, MouseButtonEvent>)
        {
            m_mouseButtonEvent.m_events.push(e);
        }
        else
        {
            static_assert("unexpected event type.");
        }
    }

    template <typename TEvent>
    void registerEventHandler(std::function<bool(const TEvent&)>&& func)
    {
        if constexpr(std::is_same_v<TEvent, KeyboardEvent>)
        {
            m_keyboardsEvent.m_handlers.push_back(std::move(func));
        }
        else if constexpr(std::is_same_v<TEvent, MouseMoveEvent>)
        {
            m_mouseMoveEvent.m_handlers.push_back(std::move(func));
        }
        else if constexpr(std::is_same_v<TEvent, MouseButtonEvent>)
        {
            m_mouseButtonEvent.m_handlers.push_back(std::move(func));
        }
        else
        {
            static_assert("unexpected event type.");
        }
    }

    bool update();
    void close();

private:
    GLFWwindow* m_window  = {};
    uint32_t    m_width   = {};
    uint32_t    m_height  = {};

private:
    template <typename TEvent>
    struct EventData
    {
        std::queue<TEvent>                              m_events;
        std::vector<std::function<bool(const TEvent&)>> m_handlers;
    };

    EventData<KeyboardEvent>    m_keyboardsEvent;
    EventData<MouseMoveEvent>   m_mouseMoveEvent;
    EventData<MouseButtonEvent> m_mouseButtonEvent;
};

}  // namespace aph

#endif  // WINDOW_H_
