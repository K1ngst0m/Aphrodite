#ifndef RENDERPASS_H_
#define RENDERPASS_H_

#include "device.h"
#include "common/spinlock.h"

namespace vkl {
using RenderPassHash = std::vector<uint64_t>;

struct RenderPassCreateInfo {
};

class VulkanRenderPass {
public:
    static VulkanRenderPass *Create(VulkanDevice *pDevice, const RenderPassCreateInfo *pCreateInfo);

    VulkanRenderPass(VkRenderPass handle, uint32_t colorAtachmentCount)
        : m_handle(handle), m_colorAttachmentCount(colorAtachmentCount) {
    }

    // const RenderPassHash &getHash() const {
    //     return m_hash;
    // }

    VkRenderPass getHandle() const {
        return m_handle;
    }

    uint32_t getColorAttachmentCount() const {
        return m_colorAttachmentCount;
    }

private:
    // RenderPassHash        m_hash                 = {};
    VkRenderPass          m_handle               = VK_NULL_HANDLE;
    uint32_t              m_colorAttachmentCount = 0;
};

} // namespace vkl

#endif // RENDERPASS_H_
