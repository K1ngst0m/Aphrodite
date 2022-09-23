#ifndef VKLTEXTURE_H_
#define VKLTEXTURE_H_

#include <vulkan/vulkan.h>

namespace vkl {
    struct VulkanTexture
    {
        VkDevice device;

        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;

        uint32_t width, height;
        uint32_t mipLevels;
        uint32_t layerCount;

        VkDescriptorImageInfo descriptorInfo;

        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        VkDeviceSize alignment = 0;
        void* mapped = nullptr;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;

        VkResult bind(VkDeviceSize offset = 0) const;

        void setupDescriptor(VkImageLayout layout);
        void destroy() const;
    };
}

#endif // VKLTEXTURE_H_
