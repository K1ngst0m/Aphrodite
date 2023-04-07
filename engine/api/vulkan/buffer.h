#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;
class VulkanQueue;

struct BufferCreateInfo
{
    uint32_t size = 0;
    uint32_t alignment = 0;
    BufferUsageFlags usage;
    MemoryPropertyFlags property;
};

class VulkanBuffer : public ResourceHandle<VkBuffer, BufferCreateInfo>
{
public:
    VulkanBuffer(const BufferCreateInfo &createInfo, VkBuffer buffer, VkDeviceMemory memory);

    uint32_t getSize() const { return m_createInfo.size; }
    uint32_t getOffset() const { return m_createInfo.alignment; }
    VkDeviceMemory getMemory() { return memory; }
    void *&getMapped() { return mapped; };

public:
    void copyTo(const void *data, VkDeviceSize size = VK_WHOLE_SIZE) const;

private:
    VkDeviceMemory memory {};
    void *mapped {};
};
}  // namespace aph

#endif  // VKLBUFFER_H_
