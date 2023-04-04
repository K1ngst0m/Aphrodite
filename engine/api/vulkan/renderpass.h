#ifndef RENDERPASS_H_
#define RENDERPASS_H_

#include "common/spinlock.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
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

}  // namespace aph

#endif  // RENDERPASS_H_
