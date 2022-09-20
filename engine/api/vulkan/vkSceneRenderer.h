#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "vkRenderable.h"
#include "vkUniformBufferObject.h"

namespace vkl {

class VulkanSceneRenderer : public SceneRenderer {
public:
    VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::Device *device, VkQueue graphics, VkQueue transfer);
    void loadResources() override;
    void cleanupResources() override;
    void update() override;
    void drawScene() override;

private:
    void _initRenderList();
    void _initUboList();
    void _setupDescriptor();

private:
    vkl::Device    *_device;
    VkCommandBuffer _drawCmd;
    VkDescriptorPool _descriptorPool;
    VkQueue _transferQueue;
    VkQueue _graphicsQueue;

private:
    std::vector<VulkanRenderable*> _renderList;
    std::vector<VulkanUniformBufferObject*> _uboList;

    VulkanUniformBufferObject* cameraUBO = nullptr;
};
}

#endif // VKSCENERENDERER_H_
