#ifndef VKLBUFFER_H_
#define VKLBUFFER_H_

#include <vulkan/vulkan.h>

namespace vkl {
    /**
    * @brief Encapsulates access to a Vulkan buffer backed up by device memory
    * @note To be filled by an external source like the VulkanDevice
    */
    struct Buffer
    {
        VkDevice device;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo descriptorInfo;
        VkDeviceSize size = 0;
        VkDeviceSize alignment = 0;
        void* mapped = nullptr;
        /** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
        VkBufferUsageFlags usageFlags;
        /** @brief Memory property flags to be filled by external source at buffer creation (to query at some later point) */
        VkMemoryPropertyFlags memoryPropertyFlags;
        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();
        VkResult bind(VkDeviceSize offset = 0) const;
        void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void copyTo(const void* data, VkDeviceSize size) const;
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        void destroy() const;
    };
}

#endif // VKLBUFFER_H_
