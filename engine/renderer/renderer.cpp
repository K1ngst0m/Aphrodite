#include "renderer.h"
#include "renderer/api/vulkan/vulkanRenderer.h"

namespace vkl {
std::unique_ptr<Renderer> Renderer::Create(RenderBackend backend, RenderConfig *config, std::shared_ptr<WindowData> windowData) {
    switch (backend) {
    case RenderBackend::VULKAN: {
        auto instance = std::make_unique<VulkanRenderer>(std::move(windowData), config);
        return instance;
    }
    case RenderBackend::OPENGL:
    default:
        assert("backend not support.");
        break;
    }
    assert("backend not support.");
    return {};
}
void Renderer::setWindowData(const std::shared_ptr<WindowData> &windowData) {
    _windowData = windowData;
}

uint32_t Renderer::getWindowHeight() {
    return _windowData->height;
};
uint32_t Renderer::getWindowWidth() {
    return _windowData->width;
};
uint32_t Renderer::getWindowAspectRation() {
    return _windowData->getAspectRatio();
}
Renderer::Renderer(std::shared_ptr<WindowData> windowData, RenderConfig *config)
    : _windowData(std::move(windowData)) {
    memcpy(&_config, config, sizeof(RenderConfig));
}
} // namespace vkl
