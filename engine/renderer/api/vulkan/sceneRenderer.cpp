#include "sceneRenderer.h"
#include "device.h"
#include "uniformBufferObject.h"
#include "renderObject.h"
#include "vkInit.hpp"
#include "pipeline.h"
#include "vulkanRenderer.h"

namespace vkl {
VulkanSceneRenderer::VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::VulkanDevice *device, VkQueue graphicsQueue, VkQueue transferQueue)
    : SceneRenderer(scene), _device(device), _drawCmd(commandBuffer), _transferQueue(transferQueue), _graphicsQueue(graphicsQueue)
{}

void VulkanSceneRenderer::loadResources() {
    _loadSceneNodes(_sceneManager->getRootNode());
    _setupDefaultShaderEffect();
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
        renderable->draw(&_globalDescriptorSet);
    }
}
void VulkanSceneRenderer::update() {
    auto & cameraUBO = _uboList[0];
    cameraUBO->updateBuffer(cameraUBO->_ubo->getData());
    // for (auto & ubo : _uboList){
    //     if (ubo->_ubo->isUpdated()){
    //         ubo->updateBuffer(ubo->_ubo->getData());
    //         ubo->_ubo->setUpdated(false);
    //     }
    // }
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

    const VkDescriptorSetAllocateInfo allocInfo  = vkl::init::descriptorSetAllocateInfo(_descriptorPool, _pass->effect->setLayouts.data(), 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &_globalDescriptorSet));

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (auto & ubo : _uboList) {
        VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = _globalDescriptorSet,
            .dstBinding      = static_cast<uint32_t>(descriptorWrites.size()),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo     = &ubo->buffer.getBufferInfo(),
        };
        descriptorWrites.push_back(write);
    }
    vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

    for (auto & renderable : _renderList) {
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
                Camera * camera  = static_cast<Camera*>(n->getObject());
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
void VulkanSceneRenderer::_setupDefaultShaderEffect() {
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
    };

    // build Shader
    std::filesystem::path shaderDir = "assets/shaders/glsl/scene_manager";
    {
        auto renderer = static_cast<VulkanRenderer*>(_renderer);
        _effect = new ShaderEffect;
        _effect->pushSetLayout(renderer->m_device->logicalDevice, perSceneBindings)
            .pushSetLayout(renderer->m_device->logicalDevice, perMaterialBindings)
            .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
            .pushShaderStages(m_shaderCache.getShaders(renderer->m_device, shaderDir / "model.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
            .pushShaderStages(m_shaderCache.getShaders(renderer->m_device, shaderDir / "model.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
            .buildPipelineLayout(renderer->m_device->logicalDevice);
        _pass = new ShaderPass;
        _pass->build(renderer->m_device->logicalDevice, renderer->m_defaultRenderPass, renderer->m_pipelineBuilder, _effect);
    }
}
} // namespace vkl
