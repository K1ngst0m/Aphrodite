#pragma once

#include "api/vulkan/vkUtils.h"
#include "common/common.h"
#include "event/event.h"
#include "event/eventManager.h"

namespace aph::vk
{
class Instance;
}

struct WindowSystemCreateInfo
{
    uint32_t width;
    uint32_t height;
    bool enableUI;
};

namespace aph
{
class WindowSystem
{
protected:
    WindowSystem(const WindowSystemCreateInfo& createInfo)
        : m_width{ createInfo.width }
        , m_height(createInfo.height)
        , m_enabledUI(createInfo.enableUI)
    {
        init();
    }

public:
    static std::unique_ptr<WindowSystem> Create(const WindowSystemCreateInfo& createInfo)
    {
        CM_LOG_INFO("Init window: [%d, %d]", createInfo.width, createInfo.height);
        return std::unique_ptr<WindowSystem>(new WindowSystem(createInfo));
    }

    virtual ~WindowSystem();

public:
    bool initUI();
    void deInitUI() const;
    uint32_t getWidth() const
    {
        return m_width;
    }
    uint32_t getHeight() const
    {
        return m_height;
    }
    void resize(uint32_t width, uint32_t height);

    std::vector<const char*> getRequiredExtensions();

    ::vk::SurfaceKHR getSurface(vk::Instance* instance);

    template <typename TEvent>
    void registerEvent(std::function<bool(const TEvent&)>&& func)
    {
        m_pEventManager->registerEvent(std::move(func));
    }

    bool update();
    void close();

protected:
    void init();

    void* m_window = {};
    uint32_t m_width = {};
    uint32_t m_height = {};
    bool m_enabledUI = {};

    std::unique_ptr<EventManager> m_pEventManager;
};

} // namespace aph
