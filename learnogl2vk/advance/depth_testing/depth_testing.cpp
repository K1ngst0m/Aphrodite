#include "depth_testing.h"

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
    .ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    .diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
    .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
};

PointLightDataLayout pointLightData{
    .position = glm::vec4(1.2f, 1.0f, 2.0f, 1.0f),
    .ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    .diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
    .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    .attenuationFactor = glm::vec4(1.0f, 0.09f, 0.032f, 0.0f),
};

std::vector<vkl::VertexLayout> planeVertices {
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
    {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},

    {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
    {{ 5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
};

void depth_testing::drawFrame()
{
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();

    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS){
        enableDepthVisualization = !enableDepthVisualization;
        glfwWaitEvents();
        vkDeviceWaitIdle(m_device->logicalDevice);
        buildCommand();
    }

    vkl::vklBase::submitFrame();
}

void depth_testing::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}

void depth_testing::cleanupDerive()
{
    m_planeMesh.destroy();
    m_model.destroy();
    sceneUBO.destroy();
    pointLightUBO.destroy();
    directionalLightUBO.destroy();

    m_modelShaderEffect.destroy(m_device->logicalDevice);
    m_modelShaderPass.destroy(m_device->logicalDevice);

    m_planeShaderEffect.destroy(m_device->logicalDevice);
    m_planeShaderPass.destroy(m_device->logicalDevice);

    m_depthShaderEffect.destroy(m_device->logicalDevice);
    m_depthShaderPass.destroy(m_device->logicalDevice);

    m_shaderCache.destory(m_device->logicalDevice);

    m_defaultScene.destroy(m_device->logicalDevice);
    m_depthScene.destroy(m_device->logicalDevice);
}

void depth_testing::updateUniformBuffer()
{
    SceneDataLayout sceneData{
        .view = m_camera.GetViewMatrix(),
        .proj = m_camera.GetProjectionMatrix(),
        .viewProj = m_camera.GetViewProjectionMatrix(),
        .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
    };
    sceneUBO.update(&sceneData);
}

void depth_testing::initDerive()
{
    loadScene();
    setupShaders();
    buildCommand();
}

void depth_testing::loadScene()
{
    {
        sceneUBO.setupBuffer(m_device, sizeof(SceneDataLayout));
        pointLightUBO.setupBuffer(m_device, sizeof(PointLightDataLayout), &pointLightData);
        directionalLightUBO.setupBuffer(m_device, sizeof(DirectionalLightDataLayout), &directionalLightData);
    }

    {
        m_model.loadFromFile(m_device, m_queues.transfer, modelDir/"FlightHelmet/glTF/FlightHelmet.gltf");
        m_planeMesh.setupMesh(m_device, m_queues.transfer, planeVertices);
        m_planeMesh.pushImage(textureDir/"metal.png", m_queues.transfer);
    }

    glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    modelTransform = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 planeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.4f, 0.0f));

    {
        m_defaultScene.pushUniform(&sceneUBO);
        m_defaultScene.pushUniform(&pointLightUBO);
        m_defaultScene.pushUniform(&directionalLightUBO);
        m_defaultScene.pushObject(&m_model, &m_modelShaderPass, modelTransform);
        m_defaultScene.pushObject(&m_planeMesh, &m_planeShaderPass, planeTransform);
    }

    {
        m_depthScene.pushUniform(&sceneUBO);
        m_depthScene.pushObject(&m_model, &m_depthShaderPass, modelTransform);
        m_depthScene.pushObject(&m_planeMesh, &m_depthShaderPass, planeTransform);
    }
}

void depth_testing::setupShaders()
{
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> globalBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };

    // depth layout
    std::vector<VkDescriptorSetLayoutBinding> depthBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
    };

    auto shaderDir = glslShaderDir/m_sessionName;

    // build Shader
    {
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, globalBindings);
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, materialBindings);
        m_modelShaderEffect.pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        m_modelShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"model.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
        m_modelShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"model.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
        m_modelShaderEffect.buildPipelineLayout(m_device->logicalDevice);

        m_modelShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_modelShaderEffect);
    }

    {
        m_planeShaderEffect.pushSetLayout(m_device->logicalDevice, globalBindings);
        m_planeShaderEffect.pushSetLayout(m_device->logicalDevice, materialBindings);
        m_planeShaderEffect.pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        m_planeShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"plane.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
        m_planeShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"plane.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
        m_planeShaderEffect.buildPipelineLayout(m_device->logicalDevice);

        m_planeShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_planeShaderEffect);
    }

    {
        m_depthShaderEffect.pushSetLayout(m_device->logicalDevice, depthBindings);
        m_depthShaderEffect.pushSetLayout(m_device->logicalDevice, materialBindings);
        m_depthShaderEffect.pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        m_depthShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"depth.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
        m_depthShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"depth.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
        m_depthShaderEffect.buildPipelineLayout(m_device->logicalDevice);

        m_depthShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_depthShaderEffect);
    }

    m_defaultScene.setupDescriptor(m_device->logicalDevice);
    m_depthScene.setupDescriptor(m_device->logicalDevice);
}

int main()
{
    depth_testing app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
void depth_testing::buildCommand()
{
    vklBase::recordCommandBuffer([&](VkCommandBuffer commandBuffer) {
        if (enableDepthVisualization) {
            m_depthScene.drawScene(commandBuffer);
        } else {
            m_defaultScene.drawScene(commandBuffer);
        }
    });
}
