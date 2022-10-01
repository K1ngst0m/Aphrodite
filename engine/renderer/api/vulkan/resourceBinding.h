#ifndef RESOURCEBINDING_H_
#define RESOURCEBINDING_H_

#include "vkUtils.h"
#include <map>

namespace vkl {
class VulkanBuffer;
class VulkanBufferView;
class VulkanImageView;

struct BindingInfo {
    VkDeviceSize      offset;
    VkDeviceSize      range;
    VulkanBuffer     *pBuffer;
    VulkanBufferView *pBufferView;
    VulkanImageView  *pImageView;
    VkSampler         sampler;
    bool              dirty;
};

using ArrayBindings = std::map<uint32_t, BindingInfo>;

struct SetBindings {
    std::unordered_map<uint32_t, ArrayBindings> bindings;
    bool                                        dirty;
};

class ResourceBindings {
public:
    bool isDirty() const {
        return m_dirty;
    }

    const std::unordered_map<uint32_t, SetBindings> &getSetBindings() {
        return m_setBindings;
    }

    void clearDirtyBit() {
        m_dirty = false;
    }

    void clear(uint32_t set);

    void reset();

    void bindBuffer(VulkanBuffer *pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement);
    void bindBufferView(VulkanBufferView *pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement);
    void bindImageView(VulkanImageView *pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
    void bindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);

private:
    void bind(uint32_t set, uint32_t binding, uint32_t arrayElement, const BindingInfo &info);

    std::unordered_map<uint32_t, SetBindings> m_setBindings;
    bool                                      m_dirty = false;
};

} // namespace vkl

#endif // RESOURCEBINDING_H_
