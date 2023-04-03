#include "framebuffer.h"
#include "device.h"

namespace vkl {
VkResult VulkanFramebuffer::Create(VulkanDevice *device, const FramebufferCreateInfo &createInfo, VulkanFramebuffer **ppFramebuffer) {
    // Iterate over attachments and make sure each ImageView exists in ObjectLookup.
    uint32_t attachmentCount = {static_cast<uint32_t>(createInfo.attachments.size())};
    const auto *ppAttachments = createInfo.attachments.data();
    std::vector<VulkanImageView *> attachments;
    for (auto i = 0U; i < attachmentCount; ++i) {
        VulkanImageView *imageView = ppAttachments[i];
        if (!imageView)
            return VK_INCOMPLETE;

        attachments.push_back(imageView);
    }

    // Create a VulkanFramebuffer class instance.
    VulkanFramebuffer *framebuffer = new VulkanFramebuffer;
    framebuffer->m_device          = device;
    framebuffer->m_createInfo     = createInfo;
    framebuffer->m_attachments = std::move(attachments);

    // Copy object address to parameter.
    *ppFramebuffer = framebuffer;

    // Return success.
    return VK_SUCCESS;
}

VkExtent2D VulkanFramebuffer::GetExtents() {
    return {m_createInfo.width, m_createInfo.height};
}

VulkanImageView *VulkanFramebuffer::GetAttachment(uint32_t attachmentIndex) {
    if (attachmentIndex < m_attachments.size())
        return m_attachments[attachmentIndex];

    return VK_NULL_HANDLE;
}

uint32_t VulkanFramebuffer::GetAttachmentCount() const {
    return static_cast<uint32_t>(m_attachments.size());
}

VkFramebuffer VulkanFramebuffer::getHandle(VulkanRenderPass *pRenderPass) {
    // Make thread-safe.
    m_spinLock.Lock();

    // See if handle already exists for given render pass.
    VkFramebuffer handle = VK_NULL_HANDLE;
    if (auto itr = m_cache.find(pRenderPass);itr != m_cache.cend()) {
        handle = itr->second;
    } else {
        // Get the native Vulkan ImageView handles from the attachment list.
        std::vector<VkImageView> attachments(m_attachments.size());
        for (auto i = 0U; i < m_attachments.size(); ++i)
            attachments[i] = m_attachments[i]->getHandle();

        // Create the Vulkan framebuffer.
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass              = pRenderPass->getHandle();
        createInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments            = attachments.data();
        createInfo.width                   = m_createInfo.width;
        createInfo.height                  = m_createInfo.height;
        createInfo.layers                  = m_createInfo.layers;

        auto result = vkCreateFramebuffer(m_device->getHandle(), &createInfo, nullptr, &handle);
        if (result == VK_SUCCESS)
            m_cache.emplace(pRenderPass, handle);
    }

    // Unlock access.
    m_spinLock.Unlock();

    return handle;
}

VulkanFramebuffer::~VulkanFramebuffer() {
    for (auto &[_, handle] : m_cache) {
        vkDestroyFramebuffer(m_device->getHandle(), handle, nullptr);
    }
}
} // namespace vkl
