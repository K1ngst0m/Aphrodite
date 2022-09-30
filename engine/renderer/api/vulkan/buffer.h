#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include "renderer/gpuResource.h"
#include <vulkan/vulkan.h>

namespace vkl {
class VulkanDevice;
class VulkanBuffer : public Buffer<VkBuffer> {
public:
    static VulkanBuffer *createFromHandle(VulkanDevice *pDevice, BufferCreateInfo *pCreateInfo, VkBuffer buffer, VkDeviceMemory memory);

    VkDeviceMemory getMemory();

public:
    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void     unmap();
    void     copyTo(const void *data, VkDeviceSize size) const;

    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

    VkResult bind(VkDeviceSize offset = 0) const;

    void                    setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkDescriptorBufferInfo &getBufferInfo();

    void destroy() const;

private:
    VkDevice               device;
    VkBuffer               buffer = VK_NULL_HANDLE;
    VkDeviceMemory         memory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo descriptorInfo;
    void                  *mapped = nullptr;
};
} // namespace vkl

#endif // VKLBUFFER_H_
