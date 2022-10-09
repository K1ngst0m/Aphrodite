#include "sceneRenderer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "device.h"
#include "framebuffer.h"
#include "pipeline.h"
#include "renderObject.h"
#include "renderpass.h"
#include "scene/camera.h"
#include "scene/entity.h"
#include "scene/light.h"
#include "scene/sceneNode.h"
#include "uniformObject.h"
#include "vkInit.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkanRenderer.h"

namespace vkl {
VulkanSceneRenderer::VulkanSceneRenderer(VulkanRenderer *renderer)
    : _device(renderer->getDevice()),
      _renderer(renderer) {
}

void VulkanSceneRenderer::loadResources() {
    _loadSceneNodes(_sceneManager->getRootNode());
    _setupUnlitShaderEffect();
    _setupDefaultLitShaderEffect();
    _initRenderList();
    _initUniformList();
    isSceneLoaded = true;
}

void VulkanSceneRenderer::cleanupResources() {
    vkDestroyDescriptorPool(_device->getHandle(), _descriptorPool, nullptr);

    for (auto &renderObject : _renderList) {
        renderObject->cleanupResources();
    }

    for (auto &ubo : _uniformList) {
        ubo->cleanupResources();
    }

    _unlitEffect->destroy(_device->getHandle());
    _unlitPass->destroy(_device->getHandle());
    _defaultLitEffect->destroy(_device->getHandle());
    _defaultLitPass->destroy(_device->getHandle());
}

void VulkanSceneRenderer::drawScene() {
    VkExtent2D extent{
        .width  = _renderer->getWindowWidth(),
        .height = _renderer->getWindowHeight(),
    };
    VkViewport viewport = vkl::init::viewport(extent);
    VkRect2D   scissor  = vkl::init::rect2D(extent);

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.pRenderPass       = _renderer->getDefaultRenderPass();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = extent;
    renderPassBeginInfo.clearValueCount   = clearValues.size();
    renderPassBeginInfo.pClearValues      = clearValues.data();

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // record command
    for (uint32_t commandIndex = 0; commandIndex < _renderer->getCommandBufferCount(); commandIndex++) {
        auto *commandBuffer = _renderer->getDefaultCommandBuffer(commandIndex);

        commandBuffer->begin(0);

        // render pass
        renderPassBeginInfo.pFramebuffer = _renderer->getDefaultFrameBuffer(commandIndex);
        commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);

        // dynamic state
        commandBuffer->cmdSetViewport(&viewport);
        commandBuffer->cmdSetSissor(&scissor);

        commandBuffer->cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, _getShaderPass()->layout, 0, 1, &_globalDescriptorSets[commandIndex]);
        commandBuffer->cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, _getShaderPass()->builtPipeline);

        for (auto &renderable : _renderList) {
            renderable->draw(_getShaderPass()->layout, commandBuffer);
        }

        commandBuffer->cmdEndRenderPass();

        commandBuffer->end();
    }
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
        renderable->loadResouces();
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

    _globalDescriptorSets.resize(_renderer->getCommandBufferCount());
    uint32_t maxSetSize = _globalDescriptorSets.size();
    for (auto &renderable : _renderList) {
        maxSetSize += renderable->getSetCount();
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

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->getHandle(), &poolInfo, nullptr, &_descriptorPool));

    const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, _getDescriptorSetLayout(SET_BINDING_SCENE), 1);
    for (auto &set : _globalDescriptorSets) {
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->getHandle(), &allocInfo, &set));
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto &uniformObj : _uniformList) {
            VkWriteDescriptorSet write = {};
            write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet               = set;
            write.dstBinding           = static_cast<uint32_t>(descriptorWrites.size());
            write.dstArrayElement      = 0;
            write.descriptorCount      = 1;
            write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo          = &uniformObj->buffer->getBufferInfo();
            descriptorWrites.push_back(write);
        }

        vkUpdateDescriptorSets(_device->getHandle(), writeCount, descriptorWrites.data(), 0, nullptr);
    }

    for (auto &renderable : _renderList) {
        renderable->setupMaterial(_getDescriptorSetLayout(SET_BINDING_MATERIAL), _descriptorPool, mtlBindingBits);
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

void VulkanSceneRenderer::_setupUnlitShaderEffect() {
    _renderer->resetPipelineBuilder();
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

    _unlitEffect = std::make_unique<VulkanPipelineLayout>();
    _unlitEffect->pushSetLayout(_device->getHandle(), perSceneBindings);
    _unlitEffect->pushSetLayout(_device->getHandle(), perMaterialBindings);
    _unlitEffect->pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    _unlitEffect->pushShaderStages(_renderer->getShaderCache().getShaders(_device, shaderDir / "unlit.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
    _unlitEffect->pushShaderStages(_renderer->getShaderCache().getShaders(_device, shaderDir / "unlit.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
    _unlitEffect->buildPipelineLayout(_device->getHandle());

    _unlitPass = std::make_unique<ShaderPass>();
    _unlitPass->buildPipeline(_device->getHandle(),
                            _renderer->getDefaultRenderPass()->getHandle(),
                            _renderer->getPipelineBuilder(),
                            _unlitEffect.get());
}

void VulkanSceneRenderer::_setupDefaultLitShaderEffect() {
    _renderer->resetPipelineBuilder();
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

    _defaultLitEffect = std::make_unique<VulkanPipelineLayout>();
    _defaultLitEffect->pushSetLayout(_device->getHandle(), perSceneBindings);
    _defaultLitEffect->pushSetLayout(_device->getHandle(), perMaterialBindings);
    _defaultLitEffect->pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    _defaultLitEffect->pushShaderStages(_renderer->getShaderCache().getShaders(_device, shaderDir / "default_lit.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
    _defaultLitEffect->pushShaderStages(_renderer->getShaderCache().getShaders(_device, shaderDir / "default_lit.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
    _defaultLitEffect->buildPipelineLayout(_device->getHandle());

    _defaultLitPass = std::make_unique<ShaderPass>();
    _defaultLitPass->buildPipeline(_device->getHandle(),
                                 _renderer->getDefaultRenderPass()->getHandle(),
                                 _renderer->getPipelineBuilder(),
                                 _defaultLitEffect.get());
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

VkDescriptorSetLayout *VulkanSceneRenderer::_getDescriptorSetLayout(DescriptorSetBinding binding) {
    return &_getShaderPass()->effect->setLayouts[binding];
}
} // namespace vkl
