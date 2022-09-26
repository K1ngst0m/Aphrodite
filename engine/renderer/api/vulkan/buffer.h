#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include <vulkan/vulkan.h>

namespace vkl {
struct VulkanBuffer {
    VkDevice               device;
    VkBuffer               buffer = VK_NULL_HANDLE;
    VkDeviceMemory         memory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo descriptorInfo;
    VkDeviceSize           size      = 0;
    VkDeviceSize           alignment = 0;
    void                  *mapped    = nullptr;
    VkBufferUsageFlags     usageFlags;
    VkMemoryPropertyFlags  memoryPropertyFlags;

    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void     unmap();
    void     copyTo(const void *data, VkDeviceSize size) const;
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

    VkResult bind(VkDeviceSize offset = 0) const;

    void                    setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkDescriptorBufferInfo &getBufferInfo();

    void destroy() const;
};
} // namespace vkl

#endif // VKLBUFFER_H_
