#pragma once

#include "api/vulkan/vkUtils.h"
#include "common/functiontraits.h"
#include "common/result.h"
#include "event/eventManager.h"
#include "global/globalManager.h"
#include "event/event.h"

namespace aph::vk
{
class Instance;
}

struct WindowSystemCreateInfo
{
    uint32_t width;
    uint32_t height;
    bool enableUI;
    bool enableHighDPI = true;
};

namespace aph
{
class WindowSystem
{
private:
    // Construction and initialization
    explicit WindowSystem(const WindowSystemCreateInfo& createInfo);
    ~WindowSystem() = default;
    auto initialize(const WindowSystemCreateInfo& createInfo) -> Result;
    void updateDPIScale();

public:
    // Factory methods
    static auto Create(const WindowSystemCreateInfo& createInfo) -> Expected<WindowSystem*>;
    static void Destroy(WindowSystem* pWindowSystem);

    // Window lifecycle
    auto update() -> bool;
    void close();
    void resize(uint32_t width, uint32_t height);

    // Window properties
    auto getWidth() const -> uint32_t;
    auto getHeight() const -> uint32_t;
    auto getPixelWidth() const -> uint32_t;
    auto getPixelHeight() const -> uint32_t;
    auto getDPIScale() const -> float;
    auto isHighDPIEnabled() const -> bool;

    // GPU integration
    auto getRequiredExtensions() -> SmallVector<const char*>;
    auto getSurface(vk::Instance* instance) -> ::vk::SurfaceKHR;
    auto getNativeHandle() -> void*;

    // Event handling
    template <typename TEvent>
    void registerEvent(TEvent&& callback);

private:
    // Member variables
    void* m_window    = {};
    uint32_t m_width  = {};
    uint32_t m_height = {};
    bool m_enableHighDPI = true;
    float m_dpiScale = 1.0f;
    EventManager& m_eventManager = APH_DEFAULT_EVENT_MANAGER;
};

template <typename TEvent>
inline void WindowSystem::registerEvent(TEvent&& callback)
{
    using traits    = FunctionTraits<std::remove_reference_t<TEvent>>;
    using eventType = typename traits::template ArgumentType<0>;

    std::function<typename traits::ReturnType(const eventType&)>&& func = APH_FWD(callback);

    m_eventManager.registerEvent(std::move(func));
}

} // namespace aph
