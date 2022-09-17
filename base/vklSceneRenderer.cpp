#include "vklSceneRenderer.h"

namespace vkl {

void SceneRenderer::setScene(SceneManager *scene) {
    _scene = scene;
    prepareResource();
}
VulkanSceneRenderer::VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::Device *device)
    : SceneRenderer(scene), _drawCmd(commandBuffer), _device(device) {
}
void VulkanSceneRenderer::prepareResource() {
    _initRenderList();
    _setupDescriptor();
}
void VulkanSceneRenderer::drawScene() {
    for (auto &renderable : _renderList) {
        vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderable.shaderPass->layout, 0, 1, &renderable.globalDescriptorSet, 0, nullptr);
        renderable.draw(_drawCmd);
    }
}
void VulkanSceneRenderer::destroy() {
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
}
void VulkanSceneRenderer::_initRenderList() {
    for (auto *renderNode : _scene->_renderNodeList) {

        Renderable renderable;
        renderable.shaderPass = renderNode->_pass;
        renderable.object     = renderNode->_entity;
        renderable.transform  = renderNode->_transform;

        _renderList.push_back(renderable);
    }
}
void VulkanSceneRenderer::_setupDescriptor() {
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(_scene->getUBOCount() * _scene->getRenderableCount())},
    };

    uint32_t maxSetSize = _scene->getRenderableCount();

    for (auto *renderNode : _scene->_renderNodeList) {
        std::vector<VkDescriptorPoolSize> setInfos = renderNode->_entity->getDescriptorSetInfo();
        for (auto &setInfo : setInfos) {
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    std::vector<VkDescriptorBufferInfo> bufferInfos{};
    if (_scene->_camera) {
        bufferInfos.push_back(_scene->_camera->_object->buffer.descriptorInfo);
    }
    for (auto *uboNode : _scene->_uniformNodeList) {
        bufferInfos.push_back(uboNode->_object->buffer.descriptorInfo);
    }

    for (size_t i = 0; i < _renderList.size(); i++) {
        auto                             *renderNode = _scene->_renderNodeList[i];
        const VkDescriptorSetAllocateInfo allocInfo  = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderNode->_pass->effect->setLayouts.data(), 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &_renderList[i].globalDescriptorSet));
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto &bufferInfo : bufferInfos) {
            VkWriteDescriptorSet write = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = _renderList[i].globalDescriptorSet,
                .dstBinding      = static_cast<uint32_t>(descriptorWrites.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo     = &bufferInfo,
            };
            descriptorWrites.push_back(write);
        }
        vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        renderNode->_entity->setupDescriptor(renderNode->_pass->effect->setLayouts[1], _descriptorPool);
    }
}
} // namespace vkl
