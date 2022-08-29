#include "lighting_maps.h"

// per scene data
// general scene data
struct SceneDataLayout {
    glm::vec4 viewPosition;
};

// point light scene data
struct PointLightDataLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

// mvp matrix data layout
struct CameraDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
};

// per material data
struct MaterialDataLayout {
    alignas(16) float shininess;
};

// per object data
struct ObjectDataLayout {
    glm::mat4 modelMatrix;
};

// vertex data layout
struct VertexDataLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexDataLayout);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

        attributeDescriptions[0] = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexDataLayout, pos),
        };

        attributeDescriptions[1] = {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexDataLayout, normal),
        };

        attributeDescriptions[2] = {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(VertexDataLayout, texCoord),
        };

        return attributeDescriptions;
    }
};


std::vector<VertexDataLayout> cubeVertices = { { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
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

std::vector<glm::vec3> cubePositions = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 5.0f, -15.0f),
                                         glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
                                         glm::vec3(2.4f, -0.4f, -3.5f), glm::vec3(-1.7f, 3.0f, -7.5f),
                                         glm::vec3(1.3f, -2.0f, -2.5f), glm::vec3(1.5f, 2.0f, -2.5f),
                                         glm::vec3(1.5f, 0.2f, -1.5f), glm::vec3(-1.3f, 1.0f, -1.5f) };

PointLightDataLayout pointLightData{
    .position = glm::vec4(1.2f, 1.0f, 2.0f, 1.0f),
    .ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    .diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
    .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
};

MaterialDataLayout materialData{
    .shininess = 128.0f,
};

void lighting_maps::drawFrame()
{
    prepareFrame();
    updateUniformBuffer(m_currentFrame);
    submitFrame();
}

void lighting_maps::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}
void lighting_maps::cleanupDerive()
{
    vkDestroyDescriptorPool(m_device->logicalDevice, m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.scene, nullptr);
    vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.material, nullptr);

    // per frame ubo
    for (size_t i = 0; i < m_settings.max_frames; i++) {
        m_mvpUBs[i].destroy();
    }

    m_cubeVB.destroy();

    m_sceneUB.destroy();
    m_materialUB.destroy();
    m_pointLightUB.destroy();

    m_containerDiffuseTexture.destroy();
    m_containerSpecularTexture.destroy();

    vkDestroyPipeline(m_device->logicalDevice, m_cubeGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_cubePipelineLayout, nullptr);

    vkDestroyPipeline(m_device->logicalDevice, m_emissionGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_emissionPipelineLayout, nullptr);
}
void lighting_maps::createVertexBuffers()
{
    VkDeviceSize bufferSize = sizeof(cubeVertices[0]) * cubeVertices.size();

    vkl::Buffer stagingBuffer;
    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    stagingBuffer.map();
    stagingBuffer.copyTo(cubeVertices.data(), static_cast<size_t>(bufferSize));
    stagingBuffer.unmap();

    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_cubeVB);

    m_device->copyBuffer(m_queues.graphics, stagingBuffer.buffer, m_cubeVB.buffer, bufferSize);

    stagingBuffer.destroy();
}
void lighting_maps::createUniformBuffers()
{
    {
        VkDeviceSize bufferSize = sizeof(CameraDataLayout);
        m_mvpUBs.resize(m_settings.max_frames);
        for (size_t i = 0; i < m_settings.max_frames; i++) {
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_mvpUBs[i]);
            m_mvpUBs[i].setupDescriptor();
        }
    }

    {
        // create scene uniform buffer
        VkDeviceSize bufferSize = sizeof(SceneDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_sceneUB);
        m_sceneUB.setupDescriptor();
    }

    {
        // create point light uniform buffer
        VkDeviceSize bufferSize = sizeof(PointLightDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_pointLightUB);
        m_pointLightUB.setupDescriptor();
    }

    {
        // create material uniform buffer
        VkDeviceSize bufferSize = sizeof(MaterialDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_materialUB);
        m_materialUB.setupDescriptor();
    }
}
void lighting_maps::createDescriptorSets()
{
    {
        std::vector<VkDescriptorSetLayout> sceneLayouts(m_swapChainImageViews.size(), m_descriptorSetLayouts.scene);
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(sceneLayouts.size()),
            .pSetLayouts = sceneLayouts.data(),
        };
        m_perFrameDescriptorSets.resize(m_settings.max_frames);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, m_perFrameDescriptorSets.data()));

        for (size_t i = 0; i < m_perFrameDescriptorSets.size(); i++) {
            std::array<VkWriteDescriptorSet, 3> descriptorWrites;

            descriptorWrites[0] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_perFrameDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &m_mvpUBs[i].descriptorInfo,
            };
            descriptorWrites[1] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_perFrameDescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &m_sceneUB.descriptorInfo,
            };
            descriptorWrites[2] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_perFrameDescriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &m_pointLightUB.descriptorInfo,
            };

            vkUpdateDescriptorSets(m_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                                   nullptr);
        }
    }

    {
        std::array<VkDescriptorSetLayout, 1> materialLayouts{ m_descriptorSetLayouts.material };

        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(materialLayouts.size()),
            .pSetLayouts = materialLayouts.data(),
        };

        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &m_cubeMaterialDescriptorSets));

        std::array<VkWriteDescriptorSet, 3> descriptorWrites;

        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_cubeMaterialDescriptorSets,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &m_materialUB.descriptorInfo,
        };

        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_cubeMaterialDescriptorSets,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &m_containerDiffuseTexture.descriptorInfo,
            .pTexelBufferView = nullptr, // Optional
        };

        descriptorWrites[2] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_cubeMaterialDescriptorSets,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &m_containerSpecularTexture.descriptorInfo,
            .pTexelBufferView = nullptr, // Optional
        };

        vkUpdateDescriptorSets(m_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                               nullptr);
    }
}
void lighting_maps::createDescriptorSetLayout()
{
    // per-scene params
    {
        VkDescriptorSetLayoutBinding cameraLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding sceneLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding pointLightLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };

        std::array<VkDescriptorSetLayoutBinding, 3> perSceneBindings = { cameraLayoutBinding, sceneLayoutBinding,
                                                                         pointLightLayoutBinding };
        VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(perSceneBindings.size()),
            .pBindings = perSceneBindings.data(),
        };
        VK_CHECK_RESULT(
                vkCreateDescriptorSetLayout(m_device->logicalDevice, &perSceneLayoutInfo, nullptr, &m_descriptorSetLayouts.scene));
    }

    // per-material params
    {
        VkDescriptorSetLayoutBinding materialLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding samplerContainerLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding samplerAwesomefaceLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };

        std::array<VkDescriptorSetLayoutBinding, 3> perMaterialBindings = { materialLayoutBinding,
                                                                            samplerContainerLayoutBinding,
                                                                            samplerAwesomefaceLayoutBinding };
        VkDescriptorSetLayoutCreateInfo perMaterialLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(perMaterialBindings.size()),
            .pBindings = perMaterialBindings.data(),
        };

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->logicalDevice, &perMaterialLayoutInfo, nullptr,
                                                    &m_descriptorSetLayouts.material));
    }
}
void lighting_maps::createGraphicsPipeline()
{
    vkl::PipelineBuilder pipelineBuilder;
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{ VertexDataLayout::getBindingDescription() };
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = VertexDataLayout::getAttributeDescriptions();
    pipelineBuilder._vertexInputInfo = vkl::init::pipelineVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);
    pipelineBuilder._inputAssembly = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    pipelineBuilder._viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)m_swapChainExtent.width,
        .height = (float)m_swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    pipelineBuilder._scissor = {
        .offset = { 0, 0 },
        .extent = m_swapChainExtent,
    };

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    pipelineBuilder._dynamicState = vkl::init::pipelineDynamicStateCreateInfo(dynamicStates.data(), static_cast<uint32_t>(dynamicStates.size()));

    pipelineBuilder._rasterizer = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder._multisampling = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    pipelineBuilder._colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    pipelineBuilder._depthStencil = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

    {
        auto vertShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "lighting/lighting_maps/cube.vert.spv");
        auto fragShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "lighting/lighting_maps/cube.frag.spv");
        VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
        pipelineBuilder._pipelineLayout = m_cubePipelineLayout;
        m_cubeGraphicsPipeline = pipelineBuilder.buildPipeline(m_device->logicalDevice, m_defaultRenderPass);
        vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
    }

    pipelineBuilder._shaderStages.clear();

    {
        auto vertShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "lighting/lighting_maps/emission.vert.spv");
        auto fragShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "lighting/lighting_maps/emission.frag.spv");
        VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
        pipelineBuilder._pipelineLayout = m_emissionPipelineLayout;
        m_emissionGraphicsPipeline = pipelineBuilder.buildPipeline(m_device->logicalDevice, m_defaultRenderPass);
        vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
    }
}
void lighting_maps::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0] = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(m_swapChainImageViews.size() * 3 + 1),
    };
    poolSizes[1] = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 2,
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = m_settings.max_frames + 1,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));
}
void lighting_maps::updateUniformBuffer(uint32_t currentFrameIndex)
{
    {
        CameraDataLayout cameraData{
            .view = m_camera.GetViewMatrix(),
            .proj = m_camera.GetProjectionMatrix(),
            .viewProj = m_camera.GetViewProjectionMatrix(),
        };

        m_mvpUBs[currentFrameIndex].map();
        m_mvpUBs[currentFrameIndex].copyTo(&cameraData, sizeof(CameraDataLayout));
        m_mvpUBs[currentFrameIndex].unmap();
    }

    {
        SceneDataLayout sceneData{
            .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
        };
        m_sceneUB.map();
        m_sceneUB.copyTo(&sceneData, sizeof(SceneDataLayout));
        m_sceneUB.unmap();
    }

    {
        m_pointLightUB.map();
        m_pointLightUB.copyTo(&pointLightData, sizeof(PointLightDataLayout));
        m_pointLightUB.unmap();
    }

    {
        m_materialUB.map();
        m_materialUB.copyTo(&materialData, sizeof(MaterialDataLayout));
        m_materialUB.unmap();
    }
}
void lighting_maps::recordCommandBuffer()
{
    for (int frameIdx = 0; frameIdx < m_swapChainImages.size(); frameIdx++){
        auto &commandBuffer = m_commandBuffers[frameIdx];
        vkResetCommandBuffer(commandBuffer, 0);
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0, // Optional
            .pInheritanceInfo = nullptr, // Optional
        };

        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };
        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = m_defaultRenderPass,
            .framebuffer = m_framebuffers[frameIdx],
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
        };
        renderPassInfo.renderArea = {
            .offset = { 0, 0 },
            .extent = m_swapChainExtent,
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(m_swapChainExtent.width),
            .height = static_cast<float>(m_swapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{
            .offset = { 0, 0 },
            .extent = m_swapChainExtent,
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { m_cubeVB.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        std::array<VkDescriptorSet, 2> descriptorSets{ m_perFrameDescriptorSets[frameIdx],
                                                       m_cubeMaterialDescriptorSets };
        // cube drawing
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_cubeGraphicsPipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_cubePipelineLayout, 0,
                                    static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

            for (size_t i = 0; i < cubePositions.size(); i++) {
                glm::mat4 model(1.0f);
                model = glm::translate(model, cubePositions[i]);
                float angle = 20.0f * i;
                model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

                ObjectDataLayout objectDataConstant{
                    .modelMatrix = model,
                };
                vkCmdPushConstants(commandBuffer, m_cubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                   sizeof(ObjectDataLayout), &objectDataConstant);

                vkCmdDraw(commandBuffer, static_cast<uint32_t>(cubeVertices.size()), 1, 0, 0);
            }
        }

        // emission drawing
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_emissionGraphicsPipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_emissionPipelineLayout, 0, 1,
                                    descriptorSets.data(), 0, nullptr);
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(1.2f, 1.0f, 2.0f));
            model = glm::scale(model, glm::vec3(0.2f));

            ObjectDataLayout objectDataConstant{
                .modelMatrix = model,
            };
            vkCmdPushConstants(commandBuffer, m_cubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectDataLayout),
                               &objectDataConstant);

            vkCmdDraw(commandBuffer, static_cast<uint32_t>(cubeVertices.size()), 1, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    }
}
void lighting_maps::createTextures()
{
    loadImageFromFile(m_containerDiffuseTexture, (textureDir / "container2.png").u8string().c_str());
    loadImageFromFile(m_containerSpecularTexture, (textureDir / "container2_specular.png").u8string().c_str());

    m_containerDiffuseTexture.view = m_device->createImageView(m_containerDiffuseTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
    m_containerSpecularTexture.view = m_device->createImageView(m_containerSpecularTexture.image, VK_FORMAT_R8G8B8A8_SRGB);

    VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = m_device->properties.limits.maxSamplerAnisotropy;
    VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerDiffuseTexture.sampler));
    VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerSpecularTexture.sampler));

    m_containerDiffuseTexture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_containerSpecularTexture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
void lighting_maps::createPipelineLayout()
{
    // cube
    {
        VkPushConstantRange objPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ObjectDataLayout),
        };

        std::array<VkPushConstantRange, 1> pushConstantRanges{
            objPushConstantRange,
        };

        std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts{ m_descriptorSetLayouts.scene,
                                                                   m_descriptorSetLayouts.material };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.data(),
        };

        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device->logicalDevice, &pipelineLayoutInfo, nullptr, &m_cubePipelineLayout));
    }

    // emission
    {
        VkPushConstantRange objPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ObjectDataLayout),
        };

        std::array<VkPushConstantRange, 1> pushConstantRanges{
            objPushConstantRange,
        };

        std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts{
            m_descriptorSetLayouts.scene,
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.data(),
        };

        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device->logicalDevice, &pipelineLayoutInfo, nullptr, &m_emissionPipelineLayout));
    }
}

int main()
{
    lighting_maps app;

    app.init();
    app.run();
    app.finish();
}
void lighting_maps::setupDescriptors()
{
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayout();
}
void lighting_maps::initDerive()
{
    createVertexBuffers();
    createUniformBuffers();
    createTextures();
    setupDescriptors();
    createSyncObjects();
    createGraphicsPipeline();
    recordCommandBuffer();
}
