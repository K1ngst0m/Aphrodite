#ifndef RENDERPASS_H_
#define RENDERPASS_H_

#include "common/spinlock.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
using RenderPassHash = std::vector<uint64_t>;
class VulkanDevice;

struct RenderPassCreateInfo
{
    std::vector<VkAttachmentDescription> colorAttachments {};
    std::optional<VkAttachmentDescription> depthAttachment {};
};

class VulkanRenderPass : public ResourceHandle<VkRenderPass>
{
public:
    static VulkanRenderPass *Create(VulkanDevice *pDevice, const RenderPassCreateInfo *pCreateInfo);

    VulkanRenderPass(VkRenderPass handle, uint32_t colorAtachmentCount) :
        m_colorAttachmentCount(colorAtachmentCount)
    {
        getHandle() = handle;
    }

    uint32_t getColorAttachmentCount() const { return m_colorAttachmentCount; }

private:
    uint32_t m_colorAttachmentCount = 0;
};

}  // namespace vkl

#endif  // RENDERPASS_H_
