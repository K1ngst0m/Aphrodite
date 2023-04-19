#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;

struct BufferCreateInfo
{
    uint32_t            size      = { 0 };
    uint32_t            alignment = { 0 };
    BufferUsageFlags    usage     = { 0 };
    MemoryPropertyFlags property  = { 0 };
};

class VulkanBuffer : public ResourceHandle<VkBuffer, BufferCreateInfo>
{
public:
    VulkanBuffer(const BufferCreateInfo& createInfo, VkBuffer buffer, VkDeviceMemory memory);

    uint32_t       getSize() const { return m_createInfo.size; }
    uint32_t       getOffset() const { return m_createInfo.alignment; }
    VkDeviceMemory getMemory() const { return memory; }
    void*&         getMapped() { return mapped; };

    void write(const void* data, size_t offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const;

private:
    VkDeviceMemory memory = {};
    void*          mapped = {};
};
}  // namespace aph

#endif  // VKLBUFFER_H_
