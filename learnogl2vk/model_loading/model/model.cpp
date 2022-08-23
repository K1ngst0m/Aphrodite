#include "model.h"

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

    vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.scene, nullptr);
    vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.material, nullptr);

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

    vkDestroyPipeline(m_device->logicalDevice, m_modelGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_modelPipelineLayout, nullptr);
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

void model::createDescriptorSets()
{
    // scene
    {
        for (size_t frameIdx = 0; frameIdx < m_settings.max_frames; frameIdx++) {
            std::vector<VkDescriptorSetLayout> sceneLayouts(1, m_descriptorSetLayouts.scene);
            VkDescriptorSetAllocateInfo allocInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_descriptorPool,
                .descriptorSetCount = static_cast<uint32_t>(sceneLayouts.size()),
                .pSetLayouts = sceneLayouts.data(),
            };
            VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &m_perFrameData[frameIdx].descriptorSet));

            std::vector<VkDescriptorBufferInfo> bufferInfos{
                m_perFrameData[frameIdx].sceneUB.descriptorInfo,
                m_perFrameData[frameIdx].pointLightUB.descriptorInfo,
                m_perFrameData[frameIdx].directionalLightUB.descriptorInfo,
            };

            std::vector<VkWriteDescriptorSet> descriptorWrites;

            for(auto & bufferInfo : bufferInfos){
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
        for (auto &image : m_cubeModel._images){
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayouts.material, 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &image.descriptorSet));
            VkWriteDescriptorSet writeDescriptorSet = vkl::init::writeDescriptorSet(image.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &image.texture.descriptorInfo);
            vkUpdateDescriptorSets(m_device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
        }
    }
}

void model::createDescriptorSetLayout()
{
    // per-scene layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        };

        VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo = vkl::init::descriptorSetLayoutCreateInfo(perSceneBindings);
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->logicalDevice, &perSceneLayoutInfo, nullptr, &m_descriptorSetLayouts.scene));
    }

    // per-material layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };

        VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo = vkl::init::descriptorSetLayoutCreateInfo(perMaterialBindings);
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->logicalDevice, &perSceneLayoutInfo, nullptr, &m_descriptorSetLayouts.material));
    }
}

void model::createGraphicsPipeline()
{
    {
        auto vertShaderCode = vkl::utils::readFile(glslShaderDir / "model_loading/model/cube.vert.spv");
        auto fragShaderCode = vkl::utils::readFile(glslShaderDir / "model_loading/model/cube.frag.spv");
        VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);
        m_pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
        m_pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
        m_pipelineBuilder._pipelineLayout = m_modelPipelineLayout;
        m_modelGraphicsPipeline = m_pipelineBuilder.buildPipeline(m_device->logicalDevice, m_renderPass);
        vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
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
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass,
        .framebuffer = m_Framebuffers[imageIndex],
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
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
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelPipelineLayout, 0,
                            static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    // cube drawing
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelGraphicsPipeline);

        ObjectDataLayout objectDataConstant{
            .modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f)),
        };

        vkCmdPushConstants(commandBuffer, m_modelPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectDataLayout), &objectDataConstant);
        m_cubeModel.draw(commandBuffer, m_modelPipelineLayout);
    }

    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void model::createPipelineLayout()
{
    VkPushConstantRange objPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(ObjectDataLayout),
    };

    std::vector<VkPushConstantRange> pushConstantRanges{
        objPushConstantRange,
    };

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ m_descriptorSetLayouts.scene, m_descriptorSetLayouts.material };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data(),
    };

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device->logicalDevice, &pipelineLayoutInfo, nullptr, &m_modelPipelineLayout));

}

void model::setupDescriptors()
{
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSets();
    createPipelineLayout();
}

void model::initDerive()
{
    loadModel();
    createUniformBuffers();
    setupDescriptors();
    createSyncObjects();
    setupPipelineBuilder();
    createGraphicsPipeline();
}

void model::loadModel()
{
    loadModelFromFile(m_cubeModel, modelDir/"FlightHelmet/glTF/FlightHelmet.gltf");
}


void model::loadModelFromFile(vkl::Model &model, const std::string& path)
{
    tinygltf::Model glTFInput;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, path);

    std::vector<uint32_t> indices;
    std::vector<vkl::VertexLayout> vertices;

    if (fileLoaded) {
        model.loadImages(m_device, m_graphicsQueue, glTFInput);
        model.loadMaterials(glTFInput);
        model.loadTextures(glTFInput);
        const tinygltf::Scene& scene = glTFInput.scenes[0];
        for (int nodeIdx : scene.nodes) {
            const tinygltf::Node node = glTFInput.nodes[nodeIdx];
            model.loadNode(node, glTFInput, nullptr, indices, vertices);
        }
    }
    else {
        assert("Could not open the glTF file.");
        return;
    }

    // Create and upload vertex and index buffer
    size_t vertexBufferSize = vertices.size() * sizeof(vkl::VertexLayout);
    size_t indexBufferSize = indices.size() * sizeof(indices[0]);

    model._mesh.setup(m_device, m_graphicsQueue, vertices, indices, vertexBufferSize, indexBufferSize);
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

