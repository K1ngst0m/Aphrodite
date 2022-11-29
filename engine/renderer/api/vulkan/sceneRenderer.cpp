#include "sceneRenderer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "common/assetManager.h"
#include "descriptorSetLayout.h"
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

struct SceneInfo {
    size_t cameraCount;
    size_t lightCount;
};

std::unordered_map<ShadingModel, MaterialBindingBits> materialBindingMap{
    {ShadingModel::UNLIT, MATERIAL_BINDING_UNLIT},
    {ShadingModel::DEFAULTLIT, MATERIAL_BINDING_DEFAULTLIT},
    {ShadingModel::PBR, MATERIAL_BINDING_PBR},
};

VulkanPipeline *CreateUnlitPipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass, SceneInfo &sceneInfo) {
    VulkanPipeline            *pipeline;
    VulkanDescriptorSetLayout *sceneLayout    = nullptr;
    VulkanDescriptorSetLayout *materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, sceneInfo.cameraCount));
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
        EffectInfo            effectInfo{};
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "unlit.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "unlit.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }

    return pipeline;
}

VulkanPipeline *CreateDefaultLitPipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass, SceneInfo &sceneInfo) {
    VulkanPipeline            *pipeline       = nullptr;
    VulkanDescriptorSetLayout *sceneLayout    = nullptr;
    VulkanDescriptorSetLayout *materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sceneInfo.cameraCount),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, sceneInfo.lightCount),
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
        EffectInfo            effectInfo{};
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "default_lit.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "default_lit.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }
    return pipeline;
}

VulkanPipeline *CreatePBRPipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass, SceneInfo &sceneInfo) {
    VulkanPipeline            *pipeline       = nullptr;
    VulkanDescriptorSetLayout *sceneLayout    = nullptr;
    VulkanDescriptorSetLayout *objectLayout   = nullptr;
    VulkanDescriptorSetLayout *materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sceneInfo.cameraCount),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, sceneInfo.lightCount),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // object
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &objectLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0), // material info
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        EffectInfo            effectInfo{};
        std::filesystem::path shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(objectLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }
    return pipeline;
}

VulkanPipeline *CreatePipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass, ShadingModel model, SceneInfo &sceneInfo) {
    switch (model) {
    case ShadingModel::UNLIT:
        return CreateUnlitPipeline(pDevice, pRenderPass, sceneInfo);
    case ShadingModel::DEFAULTLIT:
        return CreateDefaultLitPipeline(pDevice, pRenderPass, sceneInfo);
    case ShadingModel::PBR:
        return CreatePBRPipeline(pDevice, pRenderPass, sceneInfo);
    }
    return nullptr;
}

} // namespace

VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer)
    : _device(renderer->getDevice()),
      _renderer(renderer) {
}

void VulkanSceneRenderer::loadResources() {
    // _initPostFxResource();
    _loadSceneNodes();
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

    if (_forwardPipeline != nullptr) {
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

    auto  commandIndex  = _renderer->getCurrentFrameIndex();
    auto *commandBuffer = _renderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->cmdSetViewport(&viewport);
    commandBuffer->cmdSetSissor(&scissor);
    commandBuffer->cmdBindPipeline(_forwardPipeline);
    commandBuffer->cmdBindDescriptorSet(_forwardPipeline, 0, 1, &_descriptorSets[commandIndex]);

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
    for (auto &ubo : _uniformList) {
        ubo->updateBuffer(ubo->getData());
    }
}

void VulkanSceneRenderer::_initRenderList() {
    for (auto &renderable : _renderList) {
        renderable->loadResouces();
    }
}

void VulkanSceneRenderer::_initUniformList() {
    _descriptorSets.resize(_renderer->getCommandBufferCount());

    for (auto &set : _descriptorSets) {
        std::vector<VkDescriptorBufferInfo> cameraInfos{};
        std::vector<VkDescriptorBufferInfo> lightInfos{};

        for (auto &uniformData : _uniformList) {
            switch (uniformData->getNode()->getAttachType()) {
            case AttachType::LIGHT: {
                lightInfos.push_back(uniformData->getBufferInfo());
            } break;
            case AttachType::CAMERA: {
                cameraInfos.push_back(uniformData->getBufferInfo());
            } break;
            default:
                assert("invalid object type.");
            }
        }

        SceneInfo sceneInfo{
            .cameraCount = cameraInfos.size(),
            .lightCount  = lightInfos.size(),
        };
        _forwardPipeline = CreatePipeline(_device, _renderer->getDefaultRenderPass(), getShadingModel(), sceneInfo);
        set              = _forwardPipeline->getDescriptorSetLayout(SET_SCENE)->allocateSet();
        std::vector<VkWriteDescriptorSet> writes{
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, cameraInfos.data(), cameraInfos.size()),
        };

        if (getShadingModel() != ShadingModel::UNLIT) {
            writes.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, lightInfos.data(), lightInfos.size()));
        }

        vkUpdateDescriptorSets(_device->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    for (auto &renderable : _renderList) {
        renderable->setupDescriptor(_forwardPipeline->getDescriptorSetLayout(SET_OBJECT),
                                    _forwardPipeline->getDescriptorSetLayout(SET_MATERIAL),
                                    materialBindingMap[getShadingModel()]);
    }
}

void VulkanSceneRenderer::_loadSceneNodes() {
    std::queue<std::shared_ptr<SceneNode>> q;
    q.push(_scene->getRootNode());

    while (!q.empty()) {
        auto node = q.front();
        q.pop();

        switch (node->getAttachType()) {
        case AttachType::ENTITY: {
            auto renderable = std::make_shared<VulkanRenderData>(_device, node);
            _renderList.push_back(renderable);
        } break;
        case AttachType::CAMERA: {
            auto ubo = std::make_shared<VulkanUniformData>(_device, node);
            _uniformList.push_front(ubo);
        } break;
        case AttachType::LIGHT: {
            auto ubo = std::make_shared<VulkanUniformData>(_device, node);
            _uniformList.push_back(ubo);
        } break;
        default:
            assert("unattached scene node.");
            break;
        }

        for (const auto &subNode : node->getChildNode()) {
            q.push(subNode);
        }
    }
}

std::unique_ptr<VulkanSceneRenderer> VulkanSceneRenderer::Create(const std::shared_ptr<VulkanRenderer> &renderer) {
    auto instance = std::make_unique<VulkanSceneRenderer>(renderer);
    return instance;
}

VulkanDescriptorSetLayout *pSetLayout = nullptr;
} // namespace vkl
