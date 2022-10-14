#ifndef VULKAN_UNIFORMOBJECT_H_
#define VULKAN_UNIFORMOBJECT_H_

#include "buffer.h"
#include "vulkan/vulkan_core.h"

namespace vkl {
class SceneRenderer;
class VulkanDevice;
class UniformObject;

class VulkanUniformObject {
public:
    VulkanUniformObject(VulkanDevice *device, UniformObject *ubo);
    ~VulkanUniformObject() = default;

    void cleanupResources() const;

    void  setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void  updateBuffer(void *data) const;
    void *getData();

    VkDescriptorBufferInfo &getBufferInfo();

private:
    VulkanBuffer  *_buffer   = nullptr;
    VulkanDevice  *_device   = nullptr;
    UniformObject *_ubo      = nullptr;
};
} // namespace vkl

#endif // VKUNIFORMBUFFEROBJECT_H_
