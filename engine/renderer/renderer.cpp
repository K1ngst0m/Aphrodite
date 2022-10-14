#include "renderer.h"
#include "renderer/api/vulkan/vulkanRenderer.h"

namespace vkl {
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
