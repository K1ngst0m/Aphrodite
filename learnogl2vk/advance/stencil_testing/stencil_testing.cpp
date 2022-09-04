#include "stencil_testing.h"

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

void stencil_testing::drawFrame() {
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();
    vkl::vklBase::submitFrame();
}

void stencil_testing::updateUniformBuffer() {
    {
        SceneDataLayout sceneData{
            .view         = m_camera.GetViewMatrix(),
            .proj         = m_camera.GetProjectionMatrix(),
            .viewProj     = m_camera.GetViewProjectionMatrix(),
            .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
        };
        sceneUBO.update(&sceneData);
    }
}

void stencil_testing::initDerive() {
    loadScene();
    setupShaders();
    buildCommands();
}

void stencil_testing::loadScene() {
    {
        sceneUBO.setupBuffer(m_device, sizeof(SceneDataLayout));
        pointLightUBO.setupBuffer(m_device, sizeof(PointLightDataLayout), &pointLightData);
        directionalLightUBO.setupBuffer(m_device, sizeof(DirectionalLightDataLayout), &directionalLightData);
    }

    {
        m_model.loadFromFile(m_device, m_queues.transfer, modelDir / "FlightHelmet/glTF/FlightHelmet.gltf");
    }

    {
        glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));

        m_defaultScene.pushUniform(&sceneUBO)
                      .pushUniform(&pointLightUBO)
                      .pushUniform(&directionalLightUBO)
                      .pushObject(&m_model, &m_outlineShaderPass, modelTransform)
                      .pushObject(&m_model, &m_modelShaderPass, modelTransform);
    }

    m_deletionQueue.push_function([&](){
        m_model.destroy();
        sceneUBO.destroy();
        pointLightUBO.destroy();
        directionalLightUBO.destroy();
    });
}

void stencil_testing::setupShaders() {
    // per-scene layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        };
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings);
        m_outlineShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings);
    }

    // per-material layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                  VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, perMaterialBindings);
        m_outlineShaderEffect.pushSetLayout(m_device->logicalDevice, perMaterialBindings);
    }

    // push constants
    {
        m_modelShaderEffect.pushConstantRanges(
            vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        m_outlineShaderEffect.pushConstantRanges(
            vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    }

    // build Shader
    auto shaderDir = glslShaderDir / m_sessionName;
    {
        m_modelShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "model.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "model.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                           .buildPipelineLayout(m_device->logicalDevice);
        m_pipelineBuilder._depthStencil.stencilTestEnable = VK_TRUE;
        m_pipelineBuilder._depthStencil.back = {
            .failOp = VK_STENCIL_OP_REPLACE,
            .passOp = VK_STENCIL_OP_REPLACE,
            .depthFailOp = VK_STENCIL_OP_REPLACE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .compareMask = 0xff,
            .writeMask = 0xff,
            .reference = 1,
        };
        m_pipelineBuilder._depthStencil.front = m_pipelineBuilder._depthStencil.back;
        m_modelShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_modelShaderEffect);
    }

    {
        m_pipelineBuilder._depthStencil.depthTestEnable = VK_FALSE;
        m_pipelineBuilder._depthStencil.stencilTestEnable = VK_TRUE;
        m_pipelineBuilder._depthStencil.back = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_REPLACE,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_NOT_EQUAL,
            .compareMask = 0xff,
            .writeMask = 0xff,
            .reference = 1,
        };
        m_pipelineBuilder._depthStencil.front = m_pipelineBuilder._depthStencil.back;

        m_outlineShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "outline.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "outline.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                            .buildPipelineLayout(m_device->logicalDevice);
        m_outlineShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_outlineShaderEffect);
    }

    {
        m_defaultScene.setupDescriptor(m_device->logicalDevice);
    }

    m_deletionQueue.push_function([&](){
        m_modelShaderEffect.destroy(m_device->logicalDevice);
        m_modelShaderPass.destroy(m_device->logicalDevice);
        m_outlineShaderEffect.destroy(m_device->logicalDevice);
        m_outlineShaderPass.destroy(m_device->logicalDevice);
        m_shaderCache.destory(m_device->logicalDevice);

        m_defaultScene.destroy(m_device->logicalDevice);
    });
}

void stencil_testing::buildCommands() {
    for (uint32_t idx = 0; idx < m_commandBuffers.size(); idx++) {
        vklBase::recordCommandBuffer([&](VkCommandBuffer commandBuffer) {
            m_defaultScene.draw(commandBuffer);
        }, idx);
    }
}

int main() {
    stencil_testing app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
