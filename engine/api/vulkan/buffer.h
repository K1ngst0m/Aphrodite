#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class Buffer;

struct BufferCreateInfo
{
    std::size_t        size      = {0};
    std::size_t        alignment = {0};
    VkBufferUsageFlags usage     = {0};
    BufferDomain       domain    = {BufferDomain::Device};
};

class Buffer : public ResourceHandle<VkBuffer, BufferCreateInfo>
{
    friend class ObjectPool<Buffer>;
public:
    uint32_t       getSize() const { return m_createInfo.size; }
    uint32_t       getOffset() const { return m_createInfo.alignment; }
    VkDeviceMemory getMemory() const { return m_memory; }
    void*&         getMapped() { return m_mapped; };

private:
    Buffer(const CreateInfoType& createInfo, HandleType handle, VkDeviceMemory memory);
    VkDeviceMemory m_memory = {};
    void*          m_mapped = {};
};
}  // namespace aph::vk

#endif  // VKLBUFFER_H_
