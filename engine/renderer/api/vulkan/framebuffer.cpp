#include "framebuffer.h"
#include "imageView.h"
#include "renderpass.h"

namespace vkl {
VkResult VulkanFramebuffer::Create(VulkanDevice *device, const FramebufferCreateInfo *pCreateInfo, VulkanFramebuffer **ppFramebuffer, uint32_t attachmentCount, VulkanImageView **ppAttachments) {
    // Iterate over attachments and make sure each ImageView exists in ObjectLookup.
    std::vector<VulkanImageView *> attachments;
    for (auto i = 0U; i < attachmentCount; ++i) {
        VulkanImageView *imageView = ppAttachments[i];
        if (!imageView)
            return VK_INCOMPLETE;

        attachments.push_back(imageView);
    }

    // Create a VulkanFramebuffer class instance.
    VulkanFramebuffer *framebuffer = new VulkanFramebuffer;
    framebuffer->_device          = device;
    memcpy(&framebuffer->_createInfo, pCreateInfo, sizeof(FramebufferCreateInfo));
    framebuffer->_attachments = std::move(attachments);

    // Copy object address to parameter.
    *ppFramebuffer = framebuffer;

    // Return success.
    return VK_SUCCESS;
}

VkExtent2D VulkanFramebuffer::GetExtents() {
    return {_createInfo.width, _createInfo.height};
}

VulkanImageView *VulkanFramebuffer::GetAttachment(uint32_t attachmentIndex) {
    if (attachmentIndex < _attachments.size())
        return _attachments[attachmentIndex];

    return VK_NULL_HANDLE;
}

uint32_t VulkanFramebuffer::GetAttachmentCount() const {
    return static_cast<uint32_t>(_attachments.size());
}

VkFramebuffer VulkanFramebuffer::getHandle(VulkanRenderPass *pRenderPass) {
    // Make thread-safe.
    _spinLock.Lock();

    // See if handle already exists for given render pass.
    VkFramebuffer handle = VK_NULL_HANDLE;
    auto          itr    = _cache.find(pRenderPass);
    if (itr != _cache.cend()) {
        handle = itr->second;
    } else {
        // Get the native Vulkan ImageView handles from the attachment list.
        std::vector<VkImageView> attachments(_attachments.size());
        for (auto i = 0U; i < _attachments.size(); ++i)
            attachments[i] = _attachments[i]->getHandle();

        // Create the Vulkan framebuffer.
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass              = pRenderPass->getHandle();
        createInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments            = attachments.data();
        createInfo.width                   = _createInfo.width;
        createInfo.height                  = _createInfo.height;
        createInfo.layers                  = _createInfo.layers;

        auto result = vkCreateFramebuffer(_device->getLogicalDevice(), &createInfo, nullptr, &handle);
        if (result == VK_SUCCESS)
            _cache.emplace(pRenderPass, handle);
    }

    // Unlock access.
    _spinLock.Unlock();

    return handle;
}
} // namespace vkl
