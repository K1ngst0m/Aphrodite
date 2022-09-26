#ifndef VULKAN_UNIFORMBUFFEROBJECT_H_
#define VULKAN_UNIFORMBUFFEROBJECT_H_

#include "buffer.h"
#include "common.h"

namespace vkl {
class SceneRenderer;
class VulkanDevice;
class UniformBufferObject;

struct VulkanUniformBufferObject {
    VulkanUniformBufferObject(SceneRenderer *renderer, VulkanDevice *device, UniformBufferObject *ubo);
    ~VulkanUniformBufferObject() = default;

    void cleanupResources();

    vkl::VulkanBuffer buffer;

    void setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void updateBuffer(void *data);

    void destroy();

    vkl::VulkanDevice        *_device   = nullptr;
    vkl::SceneRenderer       *_renderer = nullptr;
    vkl::UniformBufferObject *_ubo      = nullptr;
};
} // namespace vkl

#endif // VKUNIFORMBUFFEROBJECT_H_
