#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "api/vulkan/buffer.h"
#include "api/vulkan/commandBuffer.h"
#include "api/vulkan/image.h"
#include "api/vulkan/shader.h"
#include "renderer/uiRenderer.h"
#include "renderer.h"

namespace aph
{
class VulkanUIRenderer : IUIRenderer
{
public:
    VulkanUIRenderer(std::shared_ptr<WindowData> windowData) :
        IUIRenderer(std::move(windowData))
    {
    }
};
}  // namespace aph

#endif  // UIRENDERER_H_
