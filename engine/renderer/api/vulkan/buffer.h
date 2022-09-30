#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include "renderer/gpuResource.h"
#include <vulkan/vulkan.h>

namespace vkl {
class VulkanBuffer {
public:
    VkDevice               device;
    VkBuffer               buffer = VK_NULL_HANDLE;
    VkDeviceMemory         memory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo descriptorInfo;
    void                  *mapped = nullptr;

    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void     unmap();
    void     copyTo(const void *data, VkDeviceSize size) const;
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

    VkResult bind(VkDeviceSize offset = 0) const;

    void                    setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkDescriptorBufferInfo &getBufferInfo();

    void destroy() const;

    BufferCreateInfo createInfo;
    uint32_t         getSize() const;
    uint32_t         getOffset() const;
};
} // namespace vkl

#endif // VKLBUFFER_H_
