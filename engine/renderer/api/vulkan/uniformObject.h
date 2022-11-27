#ifndef VULKAN_UNIFORMOBJECT_H_
#define VULKAN_UNIFORMOBJECT_H_

#include "buffer.h"
#include "scene/sceneNode.h"
#include "vulkan/vulkan_core.h"

namespace vkl {
class SceneRenderer;
class VulkanDevice;
class UniformObject;

class VulkanUniformData {
public:
    VulkanUniformData(VulkanDevice *device, std::shared_ptr<SceneNode> node);
    ~VulkanUniformData() = default;

    void cleanupResources() const;

    void  setupBuffer(uint32_t bufferSize, void *data = nullptr);
    void  updateBuffer(void *data) const;
    void *getData();

    VkDescriptorBufferInfo    &getBufferInfo();
    std::shared_ptr<SceneNode> getNode();

private:
    VulkanBuffer *_buffer = nullptr;
    VulkanDevice *_device = nullptr;

    std::shared_ptr<SceneNode>     _node = nullptr;
    std::shared_ptr<UniformObject> _ubo  = nullptr;
};
} // namespace vkl

#endif // VKUNIFORMBUFFEROBJECT_H_
