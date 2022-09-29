#include "renderer.h"
#include "renderer/api/vulkan/vulkanRenderer.h"

namespace vkl {
    std::unique_ptr<Renderer> Renderer::CreateRenderer(RenderBackend backend){
        switch (backend) {
            case RenderBackend::VULKAN:
                return std::make_unique<VulkanRenderer>();
                break;
            case RenderBackend::OPENGL:
            default:
                assert("backend not support.");
                break;

        }
        assert("backend not support.");
        return {};
    }
    void Renderer::setWindowData(const std::shared_ptr<WindowData>& windowData) {
        _windowData = windowData;
    }

    std::shared_ptr<WindowData> Renderer::getWindowData() {
        return _windowData;
    }
    } // namespace vkl
