#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include "common/spinlock.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;
class VulkanRenderPass;
class VulkanImageView;

struct FramebufferCreateInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t layers = 1;
    std::vector<VulkanImageView*> attachments;
};

class VulkanFramebuffer
{
public:
    static VkResult Create(VulkanDevice *device, const FramebufferCreateInfo &createInfo,
                           VulkanFramebuffer **ppFramebuffer);

    ~VulkanFramebuffer();

    VkFramebuffer getHandle(VulkanRenderPass *pRenderPass);

    uint32_t GetAttachmentCount() const;

    VkExtent2D GetExtents();

    VulkanImageView *GetAttachment(uint32_t attachmentIndex);

    uint32_t getWidth() const { return m_createInfo.width; }
    uint32_t getHeight() const { return m_createInfo.height; }
    uint32_t getLayerCount() const { return m_createInfo.layers; }

private:
    VulkanDevice *m_device = nullptr;
    std::vector<VulkanImageView *> m_attachments;
    std::unordered_map<VulkanRenderPass *, VkFramebuffer> m_cache;
    SpinLock m_spinLock;
    FramebufferCreateInfo m_createInfo{};
};
}  // namespace vkl

#endif  // FRAMEBUFFER_H_
