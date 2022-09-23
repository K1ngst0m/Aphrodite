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
    for (auto & renderObject : _renderList){
        renderObject->cleanupResources();
    }
    for (auto & ubo : _uboList){
        ubo->cleanupResources();
    }
}
void VulkanSceneRenderer::drawScene() {
    for (auto &renderable : _renderList) {
        renderable->draw();
    }
}
void VulkanSceneRenderer::update() {
    auto & cameraUBO = _uboList[0];
    cameraUBO->updateBuffer(cameraUBO->_ubo->getData());
    for (auto & ubo : _uboList){
        if (ubo->_ubo->isNeedUpdate()){
            ubo->updateBuffer(ubo->_ubo->getData());
            ubo->_ubo->setNeedUpdate(false);
        }
    }
}

void VulkanSceneRenderer::_initRenderList() {
    for (auto & renderable : _renderList){
        renderable->loadResouces(_transferQueue);
        renderable->setShaderPass(_pass);
    }
}
void VulkanSceneRenderer::_initUboList() {
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
    };

    uint32_t maxSetSize = 0;
    for(auto & renderable : _renderList){
        maxSetSize += renderable->getSetCount();
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    for (auto & renderable : _renderList) {
        const VkDescriptorSetAllocateInfo allocInfo  = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderable->getShaderPass()->effect->setLayouts.data(), 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &renderable->getGlobalDescriptorSet()));
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto & ubo : _uboList) {
            VkWriteDescriptorSet write = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = renderable->getGlobalDescriptorSet(),
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
                auto renderable = std::make_unique<VulkanRenderObject>(this, _device, static_cast<Entity*>(n->getObject()), _drawCmd);
                renderable->setTransform(n->getTransform());
                _renderList.push_back(std::move(renderable));
            }
            break;
        case AttachType::CAMERA:
            {
                SceneCamera * camera  = static_cast<SceneCamera*>(n->getObject());
                camera->load();
                auto cameraUBO = std::make_unique<VulkanUniformBufferObject>(this, _device, camera);
                cameraUBO->setupBuffer(camera->getDataSize(), camera->getData());
                _uboList.push_front(std::move(cameraUBO));
            }
            break;
        case AttachType::LIGHT:
            {
                Light * light  = static_cast<Light*>(n->getObject());
                light->load();
                auto ubo = std::make_unique<VulkanUniformBufferObject>(this, _device, light);
                ubo->setupBuffer(light->getDataSize(), light->getData());
                _uboList.push_back(std::move(ubo));
            }
            break;
        default:
            assert("unattached scene node.");
            break;
        }

        _loadSceneNodes(n);
    }
}

void VulkanSceneRenderer::setShaderPass(vkl::ShaderPass *pass) {
    _pass = pass;
}
} // namespace vkl
