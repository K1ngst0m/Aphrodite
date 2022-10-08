#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "renderer/uiRenderer.h"
#include "vulkanRenderer.h"

namespace vkl {
class VulkanUIRenderer : public UIRenderer {
public:
    VulkanUIRenderer(VulkanRenderer *renderer, const std::shared_ptr<WindowData>& windowData);
    void initUI() override;
    void drawUI() override;

private:
    VulkanRenderer *_renderer;
    VulkanDevice *m_device;
    DeletionQueue   m_deletionQueue;
};
} // namespace vkl

#endif // UIRENDERER_H_
