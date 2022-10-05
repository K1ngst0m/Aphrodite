#ifndef VULKAN_UNIFORMOBJECT_H_
#define VULKAN_UNIFORMOBJECT_H_

#include "buffer.h"

namespace vkl {
class SceneRenderer;
class VulkanDevice;
class UniformObject;

struct VulkanUniformObject {
    VulkanUniformObject(SceneRenderer *renderer, const std::shared_ptr<VulkanDevice>& device, UniformObject *ubo);
    ~VulkanUniformObject() = default;

    void cleanupResources() const;

    vkl::VulkanBuffer * buffer;

    void setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void updateBuffer(void *data) const;

    std::shared_ptr<VulkanDevice> _device   = nullptr;
    SceneRenderer                *_renderer = nullptr;
    UniformObject                *_ubo      = nullptr;
};
} // namespace vkl

#endif // VKUNIFORMBUFFEROBJECT_H_
