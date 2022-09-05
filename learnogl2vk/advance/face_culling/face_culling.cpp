#include "face_culling.h"

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
    {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},

    {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
    {{ 5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
};

std::vector<vkl::VertexLayout> cubeVertices = {
    // Back face
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f}}, // Bottom-left
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}}, // top-right
    {{ 0.5f, -0.5f, -0.5f},  {1.0f, 0.0f}}, // bottom-right
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}}, // top-right
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f}}, // bottom-left
    {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}}, // top-left
    // Front face
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}}, // bottom-left
    {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}}, // bottom-right
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}}, // top-right
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}}, // top-right
    {{-0.5f,  0.5f,  0.5f},  {0.0f, 1.0f}}, // top-left
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}}, // bottom-left
    // Left face
    {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}}, // top-right
    {{-0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}}, // top-left
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}}, // bottom-left
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}}, // bottom-left
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}}, // bottom-right
    {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}}, // top-right
    // Right face
     {{0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}}, // top-left
     {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}}, // bottom-right
     {{0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}}, // top-right
     {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}}, // bottom-right
     {{0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}}, // top-left
     {{0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}}, // bottom-left
    // Bottom face
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}}, // top-right
    {{ 0.5f, -0.5f, -0.5f},  {1.0f, 1.0f}}, // top-left
    {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}}, // bottom-left
    {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}}, // bottom-left
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}}, // bottom-right
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}}, // top-right
    // Top face
    {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}}, // top-left
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}}, // bottom-right
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}}, // top-right
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}}, // bottom-right
    {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}}, // top-left
    {{-0.5f,  0.5f,  0.5f},  {0.0f, 0.0f}}  // bottom-left
};

std::vector<vkl::VertexLayout> transparentVertices = {
    {{0.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f,  0.0f}},
    {{0.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f,  1.0f}},
    {{1.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {1.0f,  1.0f}},

    {{0.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f,  0.0f}},
    {{1.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {1.0f,  1.0f}},
    {{1.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {1.0f,  0.0f}}
};

void face_culling::drawFrame()
{
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();
    vkl::vklBase::submitFrame();
}

void face_culling::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}

void face_culling::updateUniformBuffer()
{
    SceneDataLayout sceneData{
        .view = m_camera.GetViewMatrix(),
        .proj = m_camera.GetProjectionMatrix(),
        .viewProj = m_camera.GetViewProjectionMatrix(),
        .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
    };
    m_sceneUBO.update(&sceneData);
}

void face_culling::initDerive()
{
    loadScene();
    setupShaders();
    buildCommands();
}

void face_culling::loadScene()
{
    {
        m_sceneUBO.setupBuffer(m_device, sizeof(SceneDataLayout));
    }

    {
        m_cubeMesh.setupMesh(m_device, m_queues.transfer, cubeVertices);
        m_cubeMesh.pushImage(textureDir/"marble.jpg", m_queues.transfer);
    }

    {
        m_defaultScene.pushCamera(&m_camera, &m_sceneUBO)
                      .pushMeshObject(&m_cubeMesh, &m_defaultShaderPass,
                                  glm::rotate(glm::mat4(1.0f), 1.25f, glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    m_deletionQueue.push_function([&](){
        m_cubeMesh.destroy();
        m_sceneUBO.destroy();
    });
}

void face_culling::setupShaders()
{
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> globalBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };

    auto shaderDir = glslShaderDir/m_sessionName;

    // build Shader
    {
        m_pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        m_pipelineBuilder._rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        m_defaultShaderEffect.pushSetLayout(m_device->logicalDevice, globalBindings)
                             .pushSetLayout(m_device->logicalDevice, materialBindings)
                             .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
                             .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"shader.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                             .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"shader.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                             .buildPipelineLayout(m_device->logicalDevice);

        m_defaultShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_defaultShaderEffect);
    }

    {
        m_defaultScene.setupDescriptor(m_device->logicalDevice);
    }

    m_deletionQueue.push_function([&](){
        m_defaultShaderEffect.destroy(m_device->logicalDevice);
        m_defaultShaderPass.destroy(m_device->logicalDevice);
        m_defaultScene.destroy(m_device->logicalDevice);
        m_shaderCache.destory(m_device->logicalDevice);
    });
}

void face_culling::buildCommands()
{
    for (uint32_t idx = 0; idx < m_commandBuffers.size(); idx++) {
        vklBase::recordCommandBuffer([&](VkCommandBuffer commandBuffer) {
            m_defaultScene.draw(commandBuffer);
        }, idx);
    }
}

int main()
{
    face_culling app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
