#include "model.h"

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

// per object data
struct ObjectDataLayout {
    glm::mat4 modelMatrix;
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

void model::drawFrame()
{
    prepareFrame();
    updateUniformBuffer(m_currentFrame);
    recordCommandBuffer(m_commandBuffers[m_currentFrame], m_imageIndices[m_currentFrame]);
    submitFrame();
}

void model::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}

void model::cleanupDerive()
{
    vkDestroyDescriptorPool(m_device->logicalDevice, m_descriptorPool, nullptr);

    m_cubeModel.destroy();

    for (auto & frameData : m_perFrameData){
        frameData.sceneUB.destroy();
        frameData.directionalLightUB.destroy();
        frameData.pointLightUB.destroy();
    }

    // perframe sync objects
    for (size_t i = 0; i < m_settings.max_frames; i++) {
        vkDestroySemaphore(m_device->logicalDevice, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device->logicalDevice, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_device->logicalDevice, m_inFlightFences[i], nullptr);
    }

    for (auto &setLayout : m_modelShaderEffect.setLayouts){
        vkDestroyDescriptorSetLayout(m_device->logicalDevice, setLayout, nullptr);
    }

    for (auto &stage : m_modelShaderEffect.stages){
        vkDestroyShaderModule(m_device->logicalDevice, stage.shaderModule, nullptr);
    }

    vkDestroyPipelineLayout(m_device->logicalDevice, m_modelShaderEffect.pipelineLayout, nullptr);

    vkDestroyPipeline(m_device->logicalDevice, m_modelShaderPass.pipeline, nullptr);
}

void model::createUniformBuffers()
{
    m_perFrameData.resize(m_settings.max_frames);

    for(auto& frameData : m_perFrameData){
        // create scene uniform buffer
        {
            VkDeviceSize bufferSize = sizeof(SceneDataLayout);
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, frameData.sceneUB);
            frameData.sceneUB.setupDescriptor();
        }

        // create point light uniform buffer
        {
            VkDeviceSize bufferSize = sizeof(PointLightDataLayout);
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, frameData.pointLightUB);
            frameData.pointLightUB.setupDescriptor();
            frameData.pointLightUB.map();
            frameData.pointLightUB.copyTo(&pointLightData, sizeof(PointLightDataLayout));
            frameData.pointLightUB.unmap();
        }

        // create directional light uniform buffer
        {
            VkDeviceSize bufferSize = sizeof(DirectionalLightDataLayout);
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, frameData.directionalLightUB);
            frameData.directionalLightUB.setupDescriptor();
            frameData.directionalLightUB.map();
            frameData.directionalLightUB.copyTo(&directionalLightData, sizeof(DirectionalLightDataLayout));
            frameData.directionalLightUB.unmap();
        }
    }

}

void model::createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_settings.max_frames * 3)},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_cubeModel._images.size())},
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = static_cast<uint32_t>(m_settings.max_frames + m_cubeModel._images.size()),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));
}

void model::updateUniformBuffer(uint32_t currentFrameIndex)
{
    {
        SceneDataLayout sceneData{
            .view = m_camera.GetViewMatrix(),
            .proj = m_camera.GetProjectionMatrix(),
            .viewProj = m_camera.GetViewProjectionMatrix(),
            .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
        };
        m_perFrameData[currentFrameIndex].sceneUB.map();
        m_perFrameData[currentFrameIndex].sceneUB.copyTo(&sceneData, sizeof(SceneDataLayout));
        m_perFrameData[currentFrameIndex].sceneUB.unmap();
    }

}

void model::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // render pass
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassInfo = vkl::init::renderPassBeginInfo(m_renderPass, clearValues, m_framebuffers[imageIndex]);
    renderPassInfo.renderArea = {
        .offset = { 0, 0 },
        .extent = m_swapChainExtent,
    };

    // dynamic state
    const VkViewport viewport = vkl::init::viewport(static_cast<float>(m_windowData.width), static_cast<float>(m_windowData.height));
    const VkRect2D scissor = vkl::init::rect2D(m_swapChainExtent);

    // vertex buffer
    VkBuffer vertexBuffers[] = { m_cubeModel._mesh.vertexBuffer.buffer };
    VkDeviceSize offsets[] = { 0 };

    // descriptor sets
    std::vector<VkDescriptorSet> descriptorSets{ m_perFrameData[m_currentFrame].descriptorSet };

    // record command
    vkResetCommandBuffer(commandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelShaderEffect.pipelineLayout, 0,
                            static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    // cube drawing
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelShaderPass.pipeline);
        m_cubeModel.draw(commandBuffer, m_modelShaderEffect.pipelineLayout);
    }

    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void model::initDerive()
{
    loadScene();
    createUniformBuffers();
    createDescriptorPool();
    createSyncObjects();
    setupPipelineBuilder();
    setupShaders();
    setupDescriptorSets();
}

void model::loadScene()
{
    loadModelFromFile(m_cubeModel, modelDir/"FlightHelmet/glTF/FlightHelmet.gltf");
}

void model::setupShaders() {
    // per-scene layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        };
        m_modelShaderEffect.pushSetLayout(m_device, perSceneBindings);
    }

    // per-material layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        m_modelShaderEffect.pushSetLayout(m_device, perMaterialBindings);
    }

    // push constants
    {
        m_modelShaderEffect.pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(ObjectDataLayout), 0));
    }

    // build Shader
    {
        auto shaderDir = glslShaderDir/m_sessionName;
        m_modelShaderEffect.build(m_device, shaderDir/"cube.vert.spv", shaderDir/"cube.frag.spv");
        m_pipelineBuilder.setShaders(m_modelShaderEffect);
        m_modelShaderPass.build(m_modelShaderEffect, m_pipelineBuilder.buildPipeline(m_device->logicalDevice, m_renderPass));
    }
}

void model::setupDescriptorSets()
{
    auto &sceneSetLayout = m_modelShaderEffect.setLayouts[DESCRIPTOR_SET_SCENE];
    auto &materialSetLayout = m_modelShaderEffect.setLayouts[DESCRIPTOR_SET_MATERIAL];

    // scene
    {
        for (size_t frameIdx = 0; frameIdx < m_settings.max_frames; frameIdx++) {
            std::vector<VkDescriptorSetLayout> sceneLayouts(1, sceneSetLayout);
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_descriptorPool, &sceneSetLayout, 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &m_perFrameData[frameIdx].descriptorSet));

            std::vector<VkDescriptorBufferInfo> bufferInfos{
                m_perFrameData[frameIdx].sceneUB.descriptorInfo,
                m_perFrameData[frameIdx].pointLightUB.descriptorInfo,
                m_perFrameData[frameIdx].directionalLightUB.descriptorInfo,
            };
            std::vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto &bufferInfo : bufferInfos) {
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_perFrameData[frameIdx].descriptorSet,
                    .dstBinding = static_cast<uint32_t>(descriptorWrites.size()),
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &bufferInfo,
                };
                descriptorWrites.push_back(write);
            }

            vkUpdateDescriptorSets(m_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    // materials
    {
        for (auto &image : m_cubeModel._images) {
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_descriptorPool, &materialSetLayout, 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &image.descriptorSet));
            VkWriteDescriptorSet writeDescriptorSet = vkl::init::writeDescriptorSet(image.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &image.texture.descriptorInfo);
            vkUpdateDescriptorSets(m_device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
        }
    }
}
void model::setupPipelineBuilder()
{
    vkl::VertexLayout::setPipelineVertexInputState({ vkl::VertexComponent::POSITION, vkl::VertexComponent::NORMAL, vkl::VertexComponent::UV, vkl::VertexComponent::COLOR });
    m_pipelineBuilder._vertexInputInfo = vkl::VertexLayout::_pipelineVertexInputStateCreateInfo;
    m_pipelineBuilder._inputAssembly = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    m_pipelineBuilder._viewport = vkl::init::viewport(static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height));
    m_pipelineBuilder._scissor = vkl::init::rect2D(m_swapChainExtent);

    m_pipelineBuilder._dynamicStages = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    m_pipelineBuilder._dynamicState = vkl::init::pipelineDynamicStateCreateInfo(m_pipelineBuilder._dynamicStages.data(), static_cast<uint32_t>(m_pipelineBuilder._dynamicStages.size()));

    m_pipelineBuilder._rasterizer = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    m_pipelineBuilder._multisampling = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    m_pipelineBuilder._colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    m_pipelineBuilder._depthStencil = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
}

int main()
{
    model app;

    app.init();
    app.run();
    app.finish();
}
