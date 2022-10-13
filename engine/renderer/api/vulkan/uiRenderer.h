#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "renderer/api/vulkan/buffer.h"
#include "renderer/uiRenderer.h"
#include "vulkanRenderer.h"

namespace vkl {
class VulkanUIRenderer : public UIRenderer {
public:
    VulkanUIRenderer(VulkanRenderer *renderer, const std::shared_ptr<WindowData> &windowData);
    void initUI() override;
    void drawUI() override;

private:
    VulkanRenderer *_renderer;
    VulkanDevice   *_device;
    DeletionQueue   _deletionQueue;
    VkQueue queue;

    VulkanBuffer * vertexBuffer;
    uint32_t vertexCount = 0;
    VulkanBuffer * indexBuffer;
    uint32_t indexCount = 0;
};
} // namespace vkl

#endif // UIRENDERER_H_
