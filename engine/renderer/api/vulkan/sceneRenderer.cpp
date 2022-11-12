#include "sceneRenderer.h"
#include "descriptorSetLayout.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "device.h"
#include "framebuffer.h"
#include "pipeline.h"
#include "renderObject.h"
#include "renderer/api/vulkan/uiRenderer.h"
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
namespace {

}

VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer>& renderer)
    : _device(renderer->getDevice()),
      _renderer(renderer) {
}

void VulkanSceneRenderer::loadResources() {
    _loadSceneNodes(_scene->getRootNode());
    _setupUnlitShaderEffect();
    _setupDefaultLitShaderEffect();
    _initRenderList();
    _initUniformList();
    isSceneLoaded = true;
}

void VulkanSceneRenderer::cleanupResources() {
    for (auto &renderObject : _renderList) {
        renderObject->cleanupResources();
    }

    for (auto &ubo : _uniformList) {
        ubo->cleanupResources();
    }

    _device->destroyPipeline(_unlitPipeline);
    _device->destroyPipeline(_defaultLitPipeline);
}

void VulkanSceneRenderer::drawScene() {
    _renderer->prepareFrame();
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

    auto commandIndex = _renderer->getCurrentFrameIndex();
    auto *commandBuffer = _renderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin(0);

    // render pass
    renderPassBeginInfo.pFramebuffer = _renderer->getDefaultFrameBuffer(_renderer->getCurrentImageIndex());
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);

    // dynamic state
    commandBuffer->cmdSetViewport(&viewport);
    commandBuffer->cmdSetSissor(&scissor);
    commandBuffer->cmdBindPipeline(_getCurrentPipeline());
    commandBuffer->cmdBindDescriptorSet(_getCurrentPipeline(), 0, 1, &_globalDescriptorSets[commandIndex]);

    for (auto &renderable : _renderList) {
        renderable->draw(_getCurrentPipeline(), commandBuffer);
    }

    commandBuffer->cmdEndRenderPass();

    commandBuffer->end();
    _renderer->submitFrame();
}

void VulkanSceneRenderer::update(float deltaTime) {
    _scene->update(deltaTime);
    auto &cameraUBO = _uniformList[0];
    cameraUBO->updateBuffer(cameraUBO->getData());

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
    _globalDescriptorSets.resize(_renderer->getCommandBufferCount());

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

    for (auto &set : _globalDescriptorSets) {
        set = _getCurrentPipeline()->getDescriptorSetLayout(SET_BINDING_SCENE)->allocateSet();
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto &uniformObj : _uniformList) {
            VkWriteDescriptorSet write = {};
            write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet               = set;
            write.dstBinding           = static_cast<uint32_t>(descriptorWrites.size());
            write.dstArrayElement      = 0;
            write.descriptorCount      = 1;
            write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo          = &uniformObj->getBufferInfo();
            descriptorWrites.push_back(write);
        }
        vkUpdateDescriptorSets(_device->getHandle(), writeCount, descriptorWrites.data(), 0, nullptr);
    }

    for (auto &renderable : _renderList) {
        renderable->setupMaterial(_getCurrentPipeline()->getDescriptorSetLayout(SET_BINDING_MATERIAL), mtlBindingBits);
    }
}

void VulkanSceneRenderer::_loadSceneNodes(const std::unique_ptr<SceneNode> &node) {
    if (node->getChildNodeCount() == 0) {
        return;
    }

    for (uint32_t idx = 0; idx < node->getChildNodeCount(); idx++) {
        auto &subNode = node->getChildNode(idx);

        switch (subNode->getAttachType()) {
        case AttachType::ENTITY: {
            auto renderable = std::make_unique<VulkanRenderData>(_device, std::static_pointer_cast<Entity>(subNode->getObject()));
            renderable->setTransform(subNode->getTransform());
            _renderList.push_back(std::move(renderable));
        } break;
        case AttachType::CAMERA: {
            auto ubo = std::make_unique<VulkanUniformData>(_device, std::static_pointer_cast<Camera>(subNode->getObject()));
            _uniformList.push_front(std::move(ubo));
        } break;
        case AttachType::LIGHT: {
            auto ubo = std::make_unique<VulkanUniformData>(_device, std::static_pointer_cast<Light>(subNode->getObject()));
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
    VulkanDescriptorSetLayout * sceneLayout = nullptr;
    VulkanDescriptorSetLayout * materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        _device->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        _device->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        EffectInfo effectInfo{};
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] = _device->getShaderCache()->getShaders(shaderDir / "unlit.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = _device->getShaderCache()->getShaders(shaderDir / "unlit.frag.spv");

        PipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(_device->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, _renderer->getDefaultRenderPass(), &_unlitPipeline));
    }
}

void VulkanSceneRenderer::_setupDefaultLitShaderEffect() {
    VulkanDescriptorSetLayout * sceneLayout = nullptr;
    VulkanDescriptorSetLayout * materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        _device->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        _device->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        EffectInfo effectInfo{};
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] = _device->getShaderCache()->getShaders(shaderDir / "default_lit.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = _device->getShaderCache()->getShaders(shaderDir / "default_lit.frag.spv");

        PipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(_device->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, _renderer->getDefaultRenderPass(), &_defaultLitPipeline));
    }
}

VulkanPipeline *VulkanSceneRenderer::_getCurrentPipeline() {
    switch (_shadingModel) {
    case ShadingModel::UNLIT:
        return _unlitPipeline;
    case ShadingModel::DEFAULTLIT:
        return _defaultLitPipeline;
    }
    assert("unexpected behavior");
    return _unlitPipeline;
}

std::unique_ptr<VulkanSceneRenderer> VulkanSceneRenderer::Create(const std::shared_ptr<VulkanRenderer>& renderer) {
    auto instance = std::make_unique<VulkanSceneRenderer>(renderer);
    return instance;
}
} // namespace vkl
