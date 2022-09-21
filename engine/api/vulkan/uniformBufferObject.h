#ifndef VKUNIFORMBUFFEROBJECT_H_
#define VKUNIFORMBUFFEROBJECT_H_

#include "scene/sceneRenderer.h"

namespace vkl {
struct VulkanUniformBufferObject{
    VulkanUniformBufferObject(vkl::SceneRenderer *renderer, vkl::Device *device, vkl::UniformBufferObject *ubo);
    ~VulkanUniformBufferObject();

    void cleanupResources();

    vkl::Buffer buffer;

    void setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void updateBuffer(void *data);

    void destroy();

    vkl::Device              *_device = nullptr;
    vkl::SceneRenderer       *_renderer = nullptr;
    vkl::UniformBufferObject *_ubo = nullptr;
};
}

#endif // VKUNIFORMBUFFEROBJECT_H_
