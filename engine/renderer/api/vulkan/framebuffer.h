#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include "device.h"
#include "common/spinlock.h"

namespace vkl {

class VulkanFramebuffer : public FrameBuffer {
public:
    static VkResult Create(VulkanDevice *device, const FramebufferCreateInfo *pCreateInfo, VulkanFramebuffer **ppFramebuffer, uint32_t attachmentCount, VulkanImageView **ppAttachments);

    ~VulkanFramebuffer() = default;

    VkFramebuffer getHandle(VulkanRenderPass *pRenderPass);

    uint32_t GetAttachmentCount() const;

    VkExtent2D GetExtents();

    VulkanImageView *GetAttachment(uint32_t attachmentIndex);

private:
    VulkanDevice                                         *_device = nullptr;
    std::vector<VulkanImageView *>                        _attachments;
    std::unordered_map<VulkanRenderPass *, VkFramebuffer> _cache;
    SpinLock                                              _spinLock;
};
} // namespace vkl

#endif // FRAMEBUFFER_H_
