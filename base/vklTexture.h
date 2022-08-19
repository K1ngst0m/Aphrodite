#ifndef VKLTEXTURE_H_
#define VKLTEXTURE_H_

#include <vulkan/vulkan.h>

namespace vkl {
    struct Texture
    {
        VkDevice device;

        VkImage image;
        VkImageView imageView;
        VkSampler sampler = VK_NULL_HANDLE;

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
