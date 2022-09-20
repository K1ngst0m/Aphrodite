#include "scene_manager.h"
#include "api/vulkan/sceneRenderer.h"

std::vector<vkl::VertexLayout> planeVertices{
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture
    // wrapping mode). this will cause the floor texture to repeat)
    {{5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},

    {{5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},
    {{5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},
};

void scene_manager::drawFrame() {
    vkl::vklApp::prepareFrame();
    updateUniformBuffer();
    vkl::vklApp::submitFrame();
}

void scene_manager::updateUniformBuffer() {
    m_sceneManager.update();
    for (auto * sceneRenderer : m_sceneRenderer){
        sceneRenderer->update();
    }
}

void scene_manager::initDerive() {
    loadScene();
    setupShaders();
    buildCommands();
}

void scene_manager::loadScene() {
    {
        m_sceneManager.setAmbient(glm::vec4(0.2f));
    }

    {
        m_sceneCamera = m_sceneManager.createCamera((float)m_windowData.width / m_windowData.height);
        auto * node = m_sceneManager.getRootNode()->createChildNode();
        node->attachObject(m_sceneCamera);
    }

    {
        m_pointLight = m_sceneManager.createLight();
        m_pointLight->setPosition({1.2f, 1.0f, 2.0f, 1.0f});
        m_pointLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_pointLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_pointLight->setType(vkl::LightType::POINT);

        m_directionalLight = m_sceneManager.createLight();
        m_directionalLight->setDirection({-0.2f, -1.0f, -0.3f, 1.0f});
        m_directionalLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_directionalLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_directionalLight->setType(vkl::LightType::DIRECTIONAL);
    }

    {
        {
            glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
            modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
            m_model = m_sceneManager.createEntity(&m_modelShaderPass);
            m_model->loadFromFile(modelDir / "FlightHelmet/glTF/FlightHelmet.gltf");

            auto * node = m_sceneManager.getRootNode()->createChildNode(modelTransform);
            node->attachObject(m_model);
        }
    }
}

void scene_manager::setupShaders() {
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };

    // build Shader
    auto shaderDir = glslShaderDir / m_sessionName;
    {
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings)
                           .pushSetLayout(m_device->logicalDevice, perMaterialBindings)
                           .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "model.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "model.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                           .buildPipelineLayout(m_device->logicalDevice);
        m_modelShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_modelShaderEffect);
    }

    {
        m_planeShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings)
                           .pushSetLayout(m_device->logicalDevice, perMaterialBindings)
                           .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "plane.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "plane.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                           .buildPipelineLayout(m_device->logicalDevice);
        m_planeShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_planeShaderEffect);
    }

    {
        m_sceneRenderer.resize(m_commandBuffers.size());
        for (size_t i = 0; i < m_sceneRenderer.size(); i++){
            m_sceneRenderer[i] = new vkl::VulkanSceneRenderer(&m_sceneManager, m_commandBuffers[i], m_device, m_queues.graphics, m_queues.transfer);
            m_sceneRenderer[i]->loadResources();
        }
    }

    m_deletionQueue.push_function([&](){
        m_modelShaderEffect.destroy(m_device->logicalDevice);
        m_modelShaderPass.destroy(m_device->logicalDevice);
        m_planeShaderEffect.destroy(m_device->logicalDevice);
        m_planeShaderPass.destroy(m_device->logicalDevice);
        m_shaderCache.destory(m_device->logicalDevice);

        for(auto* sceneRenderer : m_sceneRenderer){
            sceneRenderer->cleanupResources();
        }
    });
}

void scene_manager::buildCommands() {
    for (uint32_t idx = 0; idx < m_commandBuffers.size(); idx++) {
        vklApp::recordCommandBuffer([&](VkCommandBuffer commandBuffer) {
            m_sceneRenderer[idx]->drawScene();
        }, idx);
    }
}

int main() {
    scene_manager app;

    app.vkl::vklApp::init();
    app.vkl::vklApp::run();
    app.vkl::vklApp::finish();
}
