#ifndef WINDOW_H_
#define WINDOW_H_

#include "app/input/event.h"
#include "common.h"
#include <volk.h>

class GLFWwindow;

namespace aph::vk
{
class Instance;
}

namespace aph
{
class WSI_Glfw;
class WSI
{
protected:
    WSI(uint32_t width, uint32_t height);

public:
    template <typename TWSI>
    static std::shared_ptr<WSI> Create(uint32_t width = 800, uint32_t height = 600)
    {
        if constexpr(std::is_same_v<TWSI, WSI_Glfw>)
        {
            return std::shared_ptr<WSI>(new TWSI(width, height));
        }
        else
        {
            static_assert("unexpedted wsi type.");
        }
    }
    virtual ~WSI();

public:
    float    getAspectRatio() const { return static_cast<float>(m_width) / m_height; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }

    virtual VkSurfaceKHR getSurface(vk::Instance* instance) = 0;
    virtual uint32_t     getFrameBufferWidth() const { return m_width; };
    virtual uint32_t     getFrameBufferHeight() const { return m_height; };

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

    virtual bool update() = 0;
    virtual void close()  = 0;

protected:
    uint32_t m_width  = {};
    uint32_t m_height = {};

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

class WSI_Glfw : public WSI
{
public:
    WSI_Glfw(uint32_t width, uint32_t height);
    ~WSI_Glfw() override;
    VkSurfaceKHR getSurface(vk::Instance* instance) override;
    uint32_t     getFrameBufferWidth() const override;
    uint32_t     getFrameBufferHeight() const override;

    bool update() override;
    void close() override;

private:
    GLFWwindow* m_window = {};
};

}  // namespace aph

#endif  // WINDOW_H_
