#ifndef VKUNIFORMBUFFEROBJECT_H_
#define VKUNIFORMBUFFEROBJECT_H_

#include "common.h"
#include "buffer.h"

namespace vkl {
class SceneRenderer;
class VulkanDevice;
class UniformBufferObject;

struct VulkanUniformBufferObject{
    VulkanUniformBufferObject(SceneRenderer *renderer, VulkanDevice *device, UniformBufferObject *ubo);
    ~VulkanUniformBufferObject();

    void cleanupResources();

    vkl::VulkanBuffer buffer;

    void setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void updateBuffer(void *data);

    void destroy();

    vkl::VulkanDevice              *_device = nullptr;
    vkl::SceneRenderer       *_renderer = nullptr;
    vkl::UniformBufferObject *_ubo = nullptr;
};
}

#endif // VKUNIFORMBUFFEROBJECT_H_
