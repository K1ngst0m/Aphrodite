#include "blending.h"

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

std::vector<vkl::VertexLayout> cubeVertices = {{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
                                               { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
                                               { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                                               { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                                               { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
                                               { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },

                                               { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
                                               { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                                               { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                               { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                               { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                               { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },

                                               { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                               { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                                               { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                               { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                               { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                               { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },

                                               { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                               { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                                               { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                               { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                               { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                               { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },

                                               { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
                                               { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
                                               { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
                                               { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
                                               { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
                                               { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },

                                               { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
                                               { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                                               { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                               { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                               { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                               { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } } };

std::vector<vkl::VertexLayout> transparentVertices = {
    {{0.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f,  0.0f}},
    {{0.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f,  1.0f}},
    {{1.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {1.0f,  1.0f}},

    {{0.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f,  0.0f}},
    {{1.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {1.0f,  1.0f}},
    {{1.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {1.0f,  0.0f}}
};

void blending::drawFrame()
{
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();
    vkl::vklBase::submitFrame();
}

void blending::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}

void blending::updateUniformBuffer()
{
    SceneDataLayout sceneData{
        .view = m_camera.GetViewMatrix(),
        .proj = m_camera.GetProjectionMatrix(),
        .viewProj = m_camera.GetViewProjectionMatrix(),
        .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
    };
    m_sceneUBO.update(&sceneData);
}

void blending::initDerive()
{
    loadScene();
    setupShaders();
    buildCommands();
}

void blending::loadScene()
{
    {
        m_sceneUBO.setupBuffer(m_device, sizeof(SceneDataLayout));
    }

    {
        m_cubeMesh.setupMesh(m_device, m_queues.transfer, cubeVertices);
        m_cubeMesh.pushImage(textureDir/"marble.jpg", m_queues.transfer);

        m_planeMesh.setupMesh(m_device, m_queues.transfer, planeVertices);
        m_planeMesh.pushImage(textureDir/"metal.png", m_queues.transfer);

        m_transparentMesh.setupMesh(m_device, m_queues.transfer, transparentVertices);
        m_transparentMesh.pushImage(textureDir/"grass.png", m_queues.transfer);
    }


    {
        m_defaultScene.pushUniform(&m_sceneUBO)
                      .pushObject(&m_planeMesh, &m_defaultShaderPass)
                      .pushObject(&m_cubeMesh, &m_defaultShaderPass, glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, -1.0f)))
                      .pushObject(&m_cubeMesh, &m_defaultShaderPass, glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)));

        std::vector<glm::vec3> vegetation
        {
            glm::vec3(-1.5f, 0.0f, -0.48f),
            glm::vec3( 1.5f, 0.0f, 0.51f),
            glm::vec3( 0.0f, 0.0f, 0.7f),
            glm::vec3(-0.3f, 0.0f, -2.3f),
            glm::vec3 (0.5f, 0.0f, -0.6f)
        };

        for (auto & translate : vegetation){
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), translate);
            m_defaultScene.pushObject(&m_transparentMesh, &m_defaultShaderPass, transform);
        }
    }

    m_deletionQueue.push_function([&](){
        m_transparentMesh.destroy();
        m_planeMesh.destroy();
        m_cubeMesh.destroy();
        m_sceneUBO.destroy();
    });
}

void blending::setupShaders()
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
        m_defaultShaderEffect.pushSetLayout(m_device->logicalDevice, globalBindings)
                             .pushSetLayout(m_device->logicalDevice, materialBindings)
                             .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
                             .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"blending.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                             .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"blending.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
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

void blending::buildCommands()
{
    for (uint32_t idx = 0; idx < m_commandBuffers.size(); idx++) {
        vklBase::recordCommandBuffer([&](VkCommandBuffer commandBuffer) {
            m_defaultScene.drawScene(commandBuffer);
        }, idx);
    }
}

int main()
{
    blending app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
