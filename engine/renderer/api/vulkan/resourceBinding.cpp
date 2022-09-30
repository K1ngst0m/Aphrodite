#include "resourceBinding.h"

namespace vkl {
void ResourceBindings::clear(uint32_t set) {
    m_setBindings.erase(set);
}

void ResourceBindings::reset() {
    m_setBindings.clear();
    m_dirty = false;
}

void ResourceBindings::bindBuffer(VulkanBuffer *pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement) {
    bind(set, binding, arrayElement, BindingInfo{offset, range, pBuffer, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, true});
}

void ResourceBindings::bindBufferView(VulkanBufferView *pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement) {
    bind(set, binding, arrayElement, BindingInfo{0, 0, VK_NULL_HANDLE, pBufferView, VK_NULL_HANDLE, VK_NULL_HANDLE, true});
}

void ResourceBindings::bindImageView(VulkanImageView *pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement) {
    bind(set, binding, arrayElement, BindingInfo{0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, pImageView, sampler, true});
}

void ResourceBindings::bindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement) {
    bind(set, binding, arrayElement, BindingInfo{0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, sampler, true});
}

void ResourceBindings::bind(uint32_t set, uint32_t binding, uint32_t arrayElement, const BindingInfo &info) {
    // If resource is being removed from binding, erase the entry.
    if (info.pBuffer == VK_NULL_HANDLE && info.pBufferView == VK_NULL_HANDLE && info.pImageView == VK_NULL_HANDLE && info.sampler == VK_NULL_HANDLE) {
        auto it = m_setBindings.find(set);
        if (it != m_setBindings.end()) {
            auto &setBindings = it->second;
            auto  it2         = setBindings.bindings.find(binding);
            if (it2 != setBindings.bindings.end()) {
                auto &arrayBindings = it2->second;
                auto  it3           = arrayBindings.find(arrayElement);
                if (it3 != arrayBindings.end()) {
                    arrayBindings.erase(it3);

                    if (arrayBindings.empty())
                        setBindings.bindings.erase(it2);

                    setBindings.dirty = true;
                }
            }
        }
    }
    // Else binding is being added.
    else {
        // If set # does not exist yet, add it.
        auto it = m_setBindings.find(set);
        if (it == m_setBindings.end()) {
            ArrayBindings arrayBinding = {{arrayElement, info}};

            SetBindings setBindings;
            setBindings.bindings.emplace(binding, std::move(arrayBinding));
            setBindings.dirty = true;

            m_setBindings.emplace(set, std::move(setBindings));
        }
        // Else, find the binding #.
        else {
            // If the binding # does not exist, create it and add the array element.
            auto &setBinding = it->second;
            auto  it2        = setBinding.bindings.find(binding);
            if (it2 == setBinding.bindings.end()) {
                ArrayBindings arrayBinding = {{arrayElement, info}};
                setBinding.bindings.emplace(binding, std::move(arrayBinding));
            } else {
                it2->second[arrayElement] = info;
                setBinding.dirty          = true;
            }
        }
    }

    // Always mark ResourceBindings as dirty for fast checking during descriptor set binding.
    m_dirty = true;
}
} // namespace vkl
