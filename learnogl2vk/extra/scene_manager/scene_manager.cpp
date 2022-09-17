#include "scene_manager.h"

// per scene data
// general scene data
struct SceneDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 viewPosition;
};

// point light scene data
struct DirectionalLightDataLayout {
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

// point light scene data
struct PointLightDataLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 attenuationFactor;
};

DirectionalLightDataLayout directionalLightData{
    .direction = glm::vec4(-0.2f, -1.0f, -0.3f, 1.0f),
    .ambient   = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    .diffuse   = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
    .specular  = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
};

PointLightDataLayout pointLightData{
    .position          = glm::vec4(1.2f, 1.0f, 2.0f, 1.0f),
    .ambient           = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    .diffuse           = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
    .specular          = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    .attenuationFactor = glm::vec4(1.0f, 0.09f, 0.032f, 0.0f),
};

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
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();
    vkl::vklBase::submitFrame();
}

void scene_manager::updateUniformBuffer() {
    {
        SceneDataLayout sceneData{
            .view         = m_camera.GetViewMatrix(),
            .proj         = m_camera.GetProjectionMatrix(),
            .viewProj     = m_camera.GetViewProjectionMatrix(),
            .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
        };
        camera->update(&sceneData);
    }
}

void scene_manager::initDerive() {
    loadScene();
    setupShaders();
    buildCommands();
}

void scene_manager::loadScene() {
    {
        camera = m_sceneManager.createCamera((float)m_windowData.width / m_windowData.height);
        camera->setupBuffer(m_device, sizeof(SceneDataLayout));

        pointLight = m_sceneManager.createLight();
        pointLight->setupBuffer(m_device, sizeof(PointLightDataLayout), &pointLightData);

        directionalLight = m_sceneManager.createLight();
        directionalLight->setupBuffer(m_device, sizeof(DirectionalLightDataLayout), &directionalLightData);

        glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        m_model = m_sceneManager.createEntity(&m_modelShaderPass, modelTransform);
        m_model->loadFromFile(m_device, m_queues.transfer, modelDir / "FlightHelmet/glTF/FlightHelmet.gltf");

        glm::mat4 planeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.4f, 0.0f));
        m_plane = m_sceneManager.createEntity(&m_planeShaderPass, planeTransform);
        m_plane->setupMesh(m_device, m_queues.transfer, planeVertices);
        m_plane->pushImage(textureDir / "metal.png", m_queues.transfer);
    }

    m_deletionQueue.push_function([&](){
        m_sceneManager.destroy();
    });
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
            m_sceneRenderer[i] = new vkl::VulkanSceneRenderer(&m_sceneManager, m_commandBuffers[i], m_device);
            m_sceneRenderer[i]->prepareResource();
        }
    }

    m_deletionQueue.push_function([&](){
        m_modelShaderEffect.destroy(m_device->logicalDevice);
        m_modelShaderPass.destroy(m_device->logicalDevice);
        m_planeShaderEffect.destroy(m_device->logicalDevice);
        m_planeShaderPass.destroy(m_device->logicalDevice);
        m_shaderCache.destory(m_device->logicalDevice);
        for(auto* sceneRenderer : m_sceneRenderer){
            sceneRenderer->destroy();
        }
    });
}

void scene_manager::buildCommands() {
    for (uint32_t idx = 0; idx < m_commandBuffers.size(); idx++) {
        vklBase::recordCommandBuffer([&](VkCommandBuffer commandBuffer) {
            m_sceneRenderer[idx]->drawScene();
        }, idx);
    }
}

int main() {
    scene_manager app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
