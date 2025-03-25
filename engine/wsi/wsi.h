#pragma once

#include "api/vulkan/vkUtils.h"
#include "common/common.h"
#include "common/profiler.h"
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
        APH_PROFILER_SCOPE();
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

    SmallVector<const char*> getRequiredExtensions();

    ::vk::SurfaceKHR getSurface(vk::Instance* instance);

    template <typename TEvent>
    void registerEvent(TEvent&& callback)
    {
        using traits = FunctionTraits<std::remove_reference_t<TEvent>>;
        using event_type = typename traits::arg_type;

        std::function<typename traits::return_type(const event_type&)>&& func = APH_FWD(callback);

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
