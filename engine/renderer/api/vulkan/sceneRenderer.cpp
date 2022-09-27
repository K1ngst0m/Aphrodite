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
      _transferQueue(renderer->m_queues.transfer),
      _graphicsQueue(renderer->m_queues.graphics) {
    _initRenderResource();
}

void VulkanSceneRenderer::loadResources() {
    _loadSceneNodes(_sceneManager->getRootNode());
    _setupBaseColorShaderEffect();
    _setupPBRShaderEffect();
    _initRenderList();
    _initUboList();
    isSceneLoaded = true;
}

void VulkanSceneRenderer::cleanupResources() {
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    for (auto &renderObject : _renderList) {
        renderObject->cleanupResources();
    }
    for (auto &ubo : _uboList) {
        ubo->cleanupResources();
    }
    _unlitEffect->destroy(_device->logicalDevice);
    _unlitPass->destroy(_device->logicalDevice);
    _defaultLitEffect->destroy(_device->logicalDevice);
    _defaultLitPass->destroy(_device->logicalDevice);
    m_shaderCache.destory(_device->logicalDevice);
}

void VulkanSceneRenderer::drawScene() {
    for (uint32_t idx = 0; idx < _renderer->m_commandBuffers.size(); idx++) {
        _renderer->recordCommandBuffer(_renderer->getWindowData(), [&]() {
                for (auto &renderable : _renderList) {
                    renderable->draw(_renderer->m_commandBuffers[idx]);
                }
            }, idx);
    }
}
void VulkanSceneRenderer::update() {
    auto &cameraUBO = _uboList[0];
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
void VulkanSceneRenderer::_initUboList() {
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
    };

    uint32_t maxSetSize = 0;
    for (auto &renderable : _renderList) {
        maxSetSize += renderable->getSetCount();
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    for (auto &renderable : _renderList) {
        renderable->setupMaterial(_descriptorPool);
        renderable->setupGlobalDescriptorSet(_descriptorPool, _uboList);
    }
}
void VulkanSceneRenderer::_loadSceneNodes(std::unique_ptr<SceneNode>& node) {
    if (node->getChildNodeCount() == 0) {
        return;
    }

    for (uint32_t idx = 0; idx < node->getChildNodeCount(); idx++) {
        auto & subNode = node->getChildNode(idx);

        switch (subNode->getAttachType()) {
        case AttachType::ENTITY: {
            auto renderable = std::make_unique<VulkanRenderObject>(this, _device, static_cast<Entity *>(subNode->getObject().get()), getPrebuiltPass(static_cast<Entity*>(subNode->getObject().get())->getShadingModel()));
            renderable->setTransform(subNode->getTransform());
            _renderList.push_back(std::move(renderable));
        } break;
        case AttachType::CAMERA: {
            Camera * camera = static_cast<Camera *>(subNode->getObject().get());
            camera->load();
            auto cameraUBO = std::make_unique<VulkanUniformObject>(this, _device, camera);
            cameraUBO->setupBuffer(camera->getDataSize(), camera->getData());
            _uboList.push_front(std::move(cameraUBO));
        } break;
        case AttachType::LIGHT: {
            Light *light = static_cast<Light *>(subNode->getObject().get());
            light->load();
            auto ubo = std::make_unique<VulkanUniformObject>(this, _device, light);
            ubo->setupBuffer(light->getDataSize(), light->getData());
            _uboList.push_back(std::move(ubo));
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

    _unlitEffect       = std::make_unique<ShaderEffect>();
    _unlitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perSceneBindings);
    _unlitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perMaterialBindings);
    _unlitEffect->pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    _unlitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "basecolor.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
    _unlitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "basecolor.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
    _unlitEffect->buildPipelineLayout(_renderer->m_device->logicalDevice);

    _unlitPass = std::make_unique<ShaderPass>();
    _unlitPass->buildEffect(_renderer->m_device->logicalDevice, _renderer->m_defaultRenderPass, _renderer->m_pipelineBuilder, _unlitEffect.get());
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

    _defaultLitEffect       = std::make_unique<ShaderEffect>();
    _defaultLitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perSceneBindings);
    _defaultLitEffect->pushSetLayout(_renderer->m_device->logicalDevice, perMaterialBindings);
    _defaultLitEffect->pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    _defaultLitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "pbr.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
    _defaultLitEffect->pushShaderStages(m_shaderCache.getShaders(_renderer->m_device, shaderDir / "pbr.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
    _defaultLitEffect->buildPipelineLayout(_renderer->m_device->logicalDevice);

    _defaultLitPass = std::make_unique<ShaderPass>();
    _defaultLitPass->buildEffect(_renderer->m_device->logicalDevice, _renderer->m_defaultRenderPass, _renderer->m_pipelineBuilder, _defaultLitEffect.get());
}

void VulkanSceneRenderer::_initRenderResource() {
    _renderer->initDefaultResource();
}

std::unique_ptr<ShaderPass> &VulkanSceneRenderer::getPrebuiltPass(ShadingModel type) {
    switch (type) {
    case ShadingModel::UNLIT:
        return _unlitPass;
    case ShadingModel::DEFAULTLIT:
        return _defaultLitPass;
    }
}
} // namespace vkl
