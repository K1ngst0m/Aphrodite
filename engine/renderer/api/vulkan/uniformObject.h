#ifndef VULKAN_UNIFORMOBJECT_H_
#define VULKAN_UNIFORMOBJECT_H_

#include "buffer.h"
#include "common.h"

namespace vkl {
class SceneRenderer;
class VulkanDevice;
class UniformObject;

struct VulkanUniformObject {
    VulkanUniformObject(SceneRenderer *renderer, VulkanDevice *device, UniformObject *ubo);
    ~VulkanUniformObject() = default;

    void cleanupResources() const;

    vkl::VulkanBuffer buffer;

    void setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void updateBuffer(void *data) const;

    vkl::VulkanDevice        *_device   = nullptr;
    vkl::SceneRenderer       *_renderer = nullptr;
    vkl::UniformObject *_ubo      = nullptr;
};
} // namespace vkl

#endif // VKUNIFORMBUFFEROBJECT_H_
