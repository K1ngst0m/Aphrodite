#ifndef WSI_H_
#define WSI_H_

#include "app/input/event.h"
#include "common/common.h"
#include "api/vulkan/vkUtils.h"

namespace aph::vk
{
class Instance;
}

namespace aph
{

struct WindowHandle
{
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    Display* display;
    Window   window;
    Atom     xlib_wm_delete_window;
    Colormap colormap;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    xcb_connection_t*        connection;
    xcb_window_t             window;
    xcb_screen_t*            screen;
    xcb_intern_atom_reply_t* atom_wm_delete_window;
#elif defined(__ANDROID__)
    ANativeWindow*   window;
    ANativeActivity* activity;
    AConfiguration*  configuration;
#else
    void* window;  // hWnd
#endif
};

class WSI
{
protected:
    WSI(uint32_t width, uint32_t height) : m_width{width}, m_height(height) { init(); }

public:
    static std::unique_ptr<WSI> Create(uint32_t width = 800, uint32_t height = 600)
    {
        CM_LOG_INFO("Init window: [%d, %d]", width, height);
        return std::unique_ptr<WSI>(new WSI(width, height));
    }

    virtual ~WSI();

public:
    bool     initUI();
    void     deInitUI();
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    void     resize(uint32_t width, uint32_t height);

    std::vector<const char*> getRequiredExtensions();

    VkSurfaceKHR getSurface(vk::Instance* instance);

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
        else if constexpr(std::is_same_v<TEvent, WindowResizeEvent>)
        {
            m_windowResizeEvent.m_events.push(e);
        }
        else
        {
            CM_LOG_ERR("unexpected event type.");
            APH_ASSERT(false);
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
        else if constexpr(std::is_same_v<TEvent, WindowResizeEvent>)
        {
            m_windowResizeEvent.m_handlers.push_back(std::move(func));
        }
        else
        {
            CM_LOG_ERR("unexpected event type.");
            APH_ASSERT(false);
        }
    }

    bool update();
    void close();

protected:
    void init();

    void*    m_window = {};
    uint32_t m_width  = {};
    uint32_t m_height = {};

    template <typename TEvent>
    struct EventData
    {
        std::queue<TEvent>                              m_events;
        std::vector<std::function<bool(const TEvent&)>> m_handlers;
    };

    EventData<KeyboardEvent>     m_keyboardsEvent;
    EventData<MouseMoveEvent>    m_mouseMoveEvent;
    EventData<MouseButtonEvent>  m_mouseButtonEvent;
    EventData<WindowResizeEvent> m_windowResizeEvent;

protected:
    // TODO
    WindowHandle* m_pHandle = {};
};

}  // namespace aph

#endif  // WSI_H_
