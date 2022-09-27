#include "sceneRenderer.h"
#include "device.h"
#include "pipeline.h"
#include "renderObject.h"
#include "uniformObject.h"
#include "vkInit.hpp"
#include "vulkanRenderer.h"

namespace vkl {
VulkanSceneRenderer::VulkanSceneRenderer(VulkanRenderer *renderer)
    : _device(renderer->m_device),
      _renderer(renderer),
      _transferQueue(renderer->getDeviceQueue(DeviceQueueType::TRANSFER)),
      _graphicsQueue(renderer->getDeviceQueue(DeviceQueueType::GRAPHICS)) {
    _initRenderResource();
}

void VulkanSceneRenderer::loadResources() {
    _loadSceneNodes(_sceneManager->getRootNode());
    _setupBaseColorShaderEffect();
    _setupPBRShaderEffect();
    _initRenderList();
    _initUniformList();
    isSceneLoaded = true;
}

void VulkanSceneRenderer::cleanupResources() {
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    for (auto &renderObject : _renderList) {
        renderObject->cleanupResources();
    }
    for (auto &ubo : _uniformList) {
        ubo->cleanupResources();
    }
    _unlitEffect->destroy(_device->logicalDevice);
    _unlitPass->destroy(_device->logicalDevice);
    _defaultLitEffect->destroy(_device->logicalDevice);
    _defaultLitPass->destroy(_device->logicalDevice);
    m_shaderCache.destory(_device->logicalDevice);
}

void VulkanSceneRenderer::drawScene() {
    for (uint32_t commandIndex = 0; commandIndex < _renderer->getCommandBufferCount(); commandIndex++) {
        _renderer->recordCommandBuffer([&]() {
            auto command = _renderer->getDefaultCommandBuffers(commandIndex);
            // render pass
            VkRenderPassBeginInfo renderPassInfo = _renderer->getDefaultRenderPassCreateInfo(commandIndex);
            vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _getShaderPass()->builtPipeline);
            vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _getShaderPass()->layout, 0, 1, &_globalDescriptorSet, 0, nullptr);
            for (auto &renderable : _renderList) {
                renderable->draw(_getShaderPass()->layout, command);
            }
            vkCmdEndRenderPass(command);
        }, commandIndex);
    }
    // for (uint32_t commandIndex = 0; commandIndex < _renderer->getCommandBufferCount(); commandIndex++) {
    //     _renderer->recordCommandBuffer([&]() {
    //         auto command = _renderer->getDefaultCommandBuffers(commandIndex);
    //         // render pass
    //         VkRenderPassBeginInfo renderPassInfo = _renderer->getDefaultRenderPassCreateInfo(commandIndex);
    //         vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    //         vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _getShaderPass()->builtPipeline);
    //         vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _getShaderPass()->layout, 0, 1, &_globalDescriptorSet, 0, nullptr);
    //         for (auto &renderable : _renderList) {
    //             renderable->draw(_getShaderPass()->layout, _renderer->getDefaultCommandBuffers(commandIndex));
    //         }
    //         vkCmdEndRenderPass(command);
    //     }, commandIndex);
    // }
}
void VulkanSceneRenderer::update() {
    auto &cameraUBO = _uniformList[0];
    cameraUBO->updateBuffer(cameraUBO->_ubo->getData());
    // TODO update light data
    // for (auto & ubo : _uboList){
    //     if (ubo->_ubo->isUpdated()){
    //         ubo->updateBuffer(ubo->_ubo->getData());
    //         ubo->_ubo->setUpdated(false);
    //     }
    // }
}
void VulkanSceneRenderer::_initRenderList() {
    for (auto &renderable : _renderList) {
        renderable->loadResouces(_transferQueue);
    }
}
void VulkanSceneRenderer::_initUniformList() {
    // big fucking pool !!!
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    uint32_t maxSetSize = 1;
    for (auto &renderable : _renderList) {
        maxSetSize += renderable->getSetCount();
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, getDescriptorSetLayout(SET_BINDING_SCENE), 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &_globalDescriptorSet));

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (auto &uniformObj : _uniformList) {
        VkWriteDescriptorSet write = {};
        write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet               = _globalDescriptorSet;
        write.dstBinding           = static_cast<uint32_t>(descriptorWrites.size());
        write.dstArrayElement      = 0;
        write.descriptorCount      = 1;
        write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo          = &uniformObj->buffer.getBufferInfo();
        descriptorWrites.push_back(write);
    }

    uint32_t writeCount     = 0;
    int      mtlBindingBits = MATERIAL_BINDING_NONE;
    switch (getShadingModel()) {
    case ShadingModel::UNLIT:
        writeCount = 1;
        mtlBindingBits |= MATERIAL_BINDING_BASECOLOR;
        break;
    case ShadingModel::DEFAULTLIT:
        mtlBindingBits |= MATERIAL_BINDING_BASECOLOR | MATERIAL_BINDING_NORMAL;
        writeCount = 3;
    }

    vkUpdateDescriptorSets(_device->logicalDevice, writeCount, descriptorWrites.data(), 0, nullptr);

    for (auto &renderable : _renderList) {
        renderable->setupMaterial(getDescriptorSetLayout(SET_BINDING_MATERIAL), _descriptorPool, mtlBindingBits);
    }
}
void VulkanSceneRenderer::_loadSceneNodes(std::unique_ptr<SceneNode> &node) {
    if (node->getChildNodeCount() == 0) {
        return;
    }

    for (uint32_t idx = 0; idx < node->getChildNodeCount(); idx++) {
        auto &subNode = node->getChildNode(idx);

        switch (subNode->getAttachType()) {
        case AttachType::ENTITY: {
            auto renderable = std::make_unique<VulkanRenderObject>(this, _device, static_cast<Entity *>(subNode->getObject().get()));
            renderable->setTransform(subNode->getTransform());
            _renderList.push_back(std::move(renderable));
        } break;
        case AttachType::CAMERA: {
            Camera *camera = static_cast<Camera *>(subNode->getObject().get());
            camera->load();
            auto cameraUBO = std::make_unique<VulkanUniformObject>(this, _device, camera);
            cameraUBO->setupBuffer(camera->getDataSize(), camera->getData());
            _uniformList.push_front(std::move(cameraUBO));
        } break;
        case AttachType::LIGHT: {
            Light *light = static_cast<Light *>(subNode->getObject().get());
            light->load();
            auto ubo = std::make_unique<VulkanUniformObject>(this, _device, light);
            ubo->setupBuffer(light->getDataSize(), light->getData());
            _uniformList.push_back(std::move(ubo));
        } break;
        default:
            assert("unattached scene node.");
            break;
        }

        _loadSceneNodes(subNode);
    }
}
void VulkanSceneRenderer::_setupBaseColorShaderEffect() {
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };

    // build Shader
    std::filesystem::path shaderDir = "assets/shaders/glsl/default";

    _unlitEffect = std::make_unique<ShaderEffect>();
    _unlitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perSceneBindings);
    _unlitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perMaterialBindings);
    _unlitEffect->pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    _unlitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "unlit.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
    _unlitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "unlit.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
    _unlitEffect->buildPipelineLayout(_renderer->m_device->logicalDevice);

    _unlitPass = std::make_unique<ShaderPass>();
    _unlitPass->buildEffect(_renderer->m_device->logicalDevice, _renderer->getDefaultRenderPass(), _renderer->getPipelineBuilder(), _unlitEffect.get());
}

void VulkanSceneRenderer::_setupPBRShaderEffect() {
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
    std::filesystem::path shaderDir = "assets/shaders/glsl/default";

    _defaultLitEffect = std::make_unique<ShaderEffect>();
    _defaultLitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perSceneBindings);
    _defaultLitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perMaterialBindings);
    _defaultLitEffect->pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    _defaultLitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "default_lit.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
    _defaultLitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "default_lit.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
    _defaultLitEffect->buildPipelineLayout(_renderer->m_device->logicalDevice);

    _defaultLitPass = std::make_unique<ShaderPass>();
    _defaultLitPass->buildEffect(_renderer->m_device->logicalDevice, _renderer->getDefaultRenderPass(), _renderer->getPipelineBuilder(), _defaultLitEffect.get());
}

void VulkanSceneRenderer::_initRenderResource() {
    _renderer->initDefaultResource();
}

std::unique_ptr<ShaderPass> &VulkanSceneRenderer::_getShaderPass() {
    switch (_shadingModel) {
    case ShadingModel::UNLIT:
        return _unlitPass;
    case ShadingModel::DEFAULTLIT:
        return _defaultLitPass;
    }
    assert("unexpected behavior");
    return _unlitPass;
}
VkDescriptorSetLayout *VulkanSceneRenderer::getDescriptorSetLayout(DescriptorSetBinding binding) {
    return &_getShaderPass()->effect->setLayouts[binding];
}
} // namespace vkl
