#include "sceneRenderer.h"

namespace vkl {
VulkanSceneRenderer::VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::Device *device, VkQueue graphicsQueue, VkQueue transferQueue)
    : SceneRenderer(scene), _device(device), _drawCmd(commandBuffer), _transferQueue(transferQueue), _graphicsQueue(graphicsQueue)
{}

void VulkanSceneRenderer::loadResources() {
    _loadSceneNodes(_sceneManager->getRootNode());
    _initRenderList();
    _initUboList();
}
void VulkanSceneRenderer::cleanupResources() {
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    for (auto * renderable : _renderList){
        renderable->cleanupResources();
        delete renderable;
    }
    for (auto * ubo : _uboList){
        ubo->destroy();
        delete ubo;
    }
}
void VulkanSceneRenderer::drawScene() {
    for (auto &renderable : _renderList) {
        renderable->draw();
    }
}
void VulkanSceneRenderer::update() {
    cameraUBO->updateBuffer(cameraUBO->_ubo->getData());
    // for (auto * ubo : _uboList){
    //     if (ubo->_ubo->isNeedUpdate()){
    //         ubo->updateBuffer(ubo->_ubo->getData());
    //         ubo->_ubo->setNeedUpdate(false);
    //     }
    // }
}

void VulkanSceneRenderer::_initRenderList() {
    for (auto * renderable : _renderList){
        renderable->loadResouces(_transferQueue);
    }
}
void VulkanSceneRenderer::_initUboList() {
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(_uboList.size())},
    };

    uint32_t maxSetSize = _renderList.size();
    for (auto & renderable : _renderList) {
        std::vector<VkDescriptorPoolSize> setInfos = renderable->getDescriptorSetInfo();
        for (auto &setInfo : setInfos) {
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    for (auto & renderable : _renderList) {
        const VkDescriptorSetAllocateInfo allocInfo  = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderable->getShaderPass()->effect->setLayouts.data(), 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &renderable->getGlobalSet()));
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto * ubo : _uboList) {
            VkWriteDescriptorSet write = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = renderable->getGlobalSet(),
                .dstBinding      = static_cast<uint32_t>(descriptorWrites.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo     = &ubo->buffer.getBufferInfo(),
            };
            descriptorWrites.push_back(write);
        }
        vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        renderable->setupMaterialDescriptor(renderable->getShaderPass()->effect->setLayouts[1], _descriptorPool);
    }
}
void VulkanSceneRenderer::_loadSceneNodes(SceneNode * node) {
    if (node->getChildNodeCount() == 0){
        return;
    }

    for (uint32_t idx = 0; idx < node->getChildNodeCount(); idx++){
        auto * n = node->getChildNode(idx);

        switch (n->getAttachType()){
        case AttachType::ENTITY:
            {
                VulkanRenderable * renderable = new VulkanRenderable(this, _device, static_cast<Entity*>(n->getObject()), _drawCmd);
                renderable->setTransform(n->getTransform());
                _renderList.push_back(renderable);
            }
            break;
        case AttachType::CAMERA:
            {
                SceneCamera * camera  = static_cast<SceneCamera*>(n->getObject());
                camera->load();
                cameraUBO = new VulkanUniformBufferObject(this, _device, camera);
                cameraUBO->setupBuffer(camera->getDataSize(), camera->getData());
                _uboList.push_front(cameraUBO);
            }
            break;
        case AttachType::LIGHT:
            {
                Light * light  = static_cast<Light*>(n->getObject());
                light->load();
                VulkanUniformBufferObject * ubo = new VulkanUniformBufferObject(this, _device, light);
                ubo->setupBuffer(light->getDataSize(), light->getData());
                _uboList.push_back(ubo);
            }
            break;
        default:
            break;
        }

        _loadSceneNodes(n);
    }
}

} // namespace vkl
