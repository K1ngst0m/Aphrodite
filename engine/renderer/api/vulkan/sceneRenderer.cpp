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

bool GetShadingInfo(ShadingModel shadingModel, uint32_t& writeCount, int32_t& mtlBindingBits){
    switch (shadingModel) {
    case ShadingModel::UNLIT:
        writeCount = 1;
        mtlBindingBits |= MATERIAL_BINDING_BASECOLOR;
        return true;
    case ShadingModel::DEFAULTLIT:
        mtlBindingBits |= MATERIAL_BINDING_BASECOLOR | MATERIAL_BINDING_NORMAL;
        writeCount = 3;
        return true;
    }
    return false;
}

VulkanPipeline * CreateUnlitPipeline(VulkanDevice * pDevice, VulkanRenderPass * pRenderPass) {
    VulkanPipeline * pipeline;
    VulkanDescriptorSetLayout * sceneLayout = nullptr;
    VulkanDescriptorSetLayout * materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        EffectInfo effectInfo{};
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "unlit.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "unlit.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }

    return pipeline;
}

VulkanPipeline* CreateDefaultLitPipeline(VulkanDevice * pDevice, VulkanRenderPass * pRenderPass) {
    VulkanPipeline * pipeline = nullptr;
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
        pDevice->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        EffectInfo effectInfo{};
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "default_lit.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "default_lit.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }
    return pipeline;
}

VulkanPipeline* CreatePipeline(VulkanDevice * pDevice, VulkanRenderPass * pRenderPass, ShadingModel model){
    switch(model){
    case ShadingModel::UNLIT:
        return CreateUnlitPipeline(pDevice, pRenderPass);
    case ShadingModel::DEFAULTLIT:
        return CreateDefaultLitPipeline(pDevice, pRenderPass);
    }
    return nullptr;
}

}

VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer>& renderer)
    : _device(renderer->getDevice()),
      _renderer(renderer) {
}

void VulkanSceneRenderer::loadResources() {
    // _initPostFxResource();
    _loadSceneNodes();
    _forwardPipeline = CreatePipeline(_device, _renderer->getDefaultRenderPass(), getShadingModel());
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

    if (_forwardPipeline != nullptr){
        _device->destroyPipeline(_forwardPipeline);
    }
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

    commandBuffer->begin();

    // dynamic state
    commandBuffer->cmdSetViewport(&viewport);
    commandBuffer->cmdSetSissor(&scissor);
    commandBuffer->cmdBindPipeline(_forwardPipeline);
    commandBuffer->cmdBindDescriptorSet(_forwardPipeline, 0, 1, &_globalDescriptorSets[commandIndex]);

    // forward pass
    renderPassBeginInfo.pFramebuffer = _renderer->getDefaultFrameBuffer(_renderer->getCurrentImageIndex());
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);
    for (auto &renderable : _renderList) {
        renderable->draw(_forwardPipeline, commandBuffer);
    }

    commandBuffer->cmdEndRenderPass();

    commandBuffer->end();
    _renderer->submitAndPresent();
}

void VulkanSceneRenderer::update(float deltaTime) {
    for (auto & ubo : _uniformList){
        ubo->updateBuffer(ubo->getData());
    }
}

void VulkanSceneRenderer::_initRenderList() {
    for (auto &renderable : _renderList) {
        renderable->loadResouces();
    }
}

void VulkanSceneRenderer::_initUniformList() {
    _globalDescriptorSets.resize(_renderer->getCommandBufferCount());

    uint32_t writeCount     = 0;
    int32_t  mtlBindingBits = MATERIAL_BINDING_NONE;
    GetShadingInfo(getShadingModel(), writeCount, mtlBindingBits);

    for (auto &set : _globalDescriptorSets) {
        set = _forwardPipeline->getDescriptorSetLayout(SET_BINDING_SCENE)->allocateSet();
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
        renderable->setupMaterial(_forwardPipeline->getDescriptorSetLayout(SET_BINDING_MATERIAL), mtlBindingBits);
    }
}

void VulkanSceneRenderer::_loadSceneNodes() {
    std::queue<std::shared_ptr<SceneNode>> q;
    q.push(_scene->getRootNode());

    while (!q.empty()){
        auto node = q.front();
        q.pop();

        switch (node->getAttachType()) {
        case AttachType::ENTITY: {
            auto renderable = std::make_shared<VulkanRenderData>(_device, node);
            _renderList.push_back(renderable);
        } break;
        case AttachType::CAMERA: {
            auto ubo = std::make_shared<VulkanUniformData>(_device, node->getObject<Camera>());
            _uniformList.push_front(ubo);
        } break;
        case AttachType::LIGHT: {
            auto ubo = std::make_shared<VulkanUniformData>(_device, node->getObject<Light>());
            _uniformList.push_back(ubo);
        } break;
        default:
            assert("unattached scene node.");
            break;
        }

        for (const auto& subNode : node->getChildNode()){
            q.push(subNode);
        }
    }

}

std::unique_ptr<VulkanSceneRenderer> VulkanSceneRenderer::Create(const std::shared_ptr<VulkanRenderer>& renderer) {
    auto instance = std::make_unique<VulkanSceneRenderer>(renderer);
    return instance;
}

void VulkanSceneRenderer::_initPostFxResource() {
    VulkanDescriptorSetLayout * pSetLayout = nullptr;

    // shader resource
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            // Binding 0: Input image (read-only)
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
            // Binding 1: Output image (write)
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1),
        };

        VkDescriptorSetLayoutCreateInfo setLayoutCI = vkl::init::descriptorSetLayoutCreateInfo(setLayoutBindings);
        _device->createDescriptorSetLayout(&setLayoutCI, &pSetLayout);
    }

    // pipeline
    {
        EffectInfo info{};
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        info.shaderMapList[VK_SHADER_STAGE_COMPUTE_BIT] = _device->getShaderCache()->getShaders(shaderDir / "postFx.comp");
        info.setLayouts.push_back(pSetLayout);
        _device->createComputePipeline(&info, &_postFxResource.pipeline);
    }
}

} // namespace vkl
