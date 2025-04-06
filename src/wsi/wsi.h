#pragma once

#include "api/vulkan/vkUtils.h"
#include "common/common.h"
#include "common/functiontraits.h"
#include "common/profiler.h"
#include "common/result.h"
#include "event/event.h"
#include "event/eventManager.h"
#include "exception/errorMacros.h"
#include "global/globalManager.h"

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
private:
    WindowSystem(const WindowSystemCreateInfo& createInfo)
        : m_width{ createInfo.width }
        , m_height(createInfo.height)
    {
    }
    ~WindowSystem() = default;

    Result initialize(const WindowSystemCreateInfo& createInfo);

public:
    // Factory methods
    static Expected<WindowSystem*> Create(const WindowSystemCreateInfo& createInfo);
    static void Destroy(WindowSystem* pWindowSystem);

public:
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

    void* getNativeHandle();

    template <typename TEvent>
    void registerEvent(TEvent&& callback)
    {
        using traits = FunctionTraits<std::remove_reference_t<TEvent>>;
        using eventType = typename traits::template ArgumentType<0>;

        std::function<typename traits::ReturnType(const eventType&)>&& func = APH_FWD(callback);

        m_eventManager.registerEvent(std::move(func));
    }

    bool update();
    void close();

private:
    void* m_window = {};
    uint32_t m_width = {};
    uint32_t m_height = {};

    EventManager& m_eventManager = APH_DEFAULT_EVENT_MANAGER;
};

} // namespace aph
