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
    std::vector<VkAttachmentDescription> depthAttachment {};
};

class VulkanRenderPass
{
public:
    static VulkanRenderPass *Create(VulkanDevice *pDevice, const RenderPassCreateInfo *pCreateInfo);

    VulkanRenderPass(VkRenderPass handle, uint32_t colorAtachmentCount) :
        m_handle(handle),
        m_colorAttachmentCount(colorAtachmentCount)
    {
    }

    VkRenderPass getHandle() const { return m_handle; }

    uint32_t getColorAttachmentCount() const { return m_colorAttachmentCount; }

private:
    VkRenderPass m_handle = VK_NULL_HANDLE;
    uint32_t m_colorAttachmentCount = 0;
};

}  // namespace vkl

#endif  // RENDERPASS_H_
