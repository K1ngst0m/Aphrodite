#include "mesh.h"

// per scene data
// general scene data
struct SceneDataLayout {
    glm::vec4 viewPosition;
};

// flash light data
struct FlashLightDataLayout {
    glm::vec4 position;
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    alignas(4) float cutOff;
    alignas(4) float outerCutOff;
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

// mvp matrix data layout
struct CameraDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
};

// per object data
struct ObjectDataLayout {
    glm::mat4 modelMatrix;
};


std::vector<vkl::VertexLayout> cubeVertices = { { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
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

void mesh::drawFrame()
{
    prepareFrame();
    updateUniformBuffer(m_currentFrame);
    recordCommandBuffer();
    submitFrame();
}

void mesh::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}
void mesh::cleanupDerive()
{
    vkDestroyDescriptorPool(m_device->logicalDevice, m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.scene, nullptr);
    vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.material, nullptr);

    // per frame ubo
    for (size_t i = 0; i < m_settings.max_frames; i++) {
        m_mvpUBs[i].destroy();
    }

    m_cubeMesh.destroy();

    m_sceneUB.destroy();
    m_materialUB.destroy();

    m_directionalLightUB.destroy();
    m_pointLightUB.destroy();
    m_flashLightUB.destroy();

    m_containerDiffuseTexture.destroy();
    m_containerSpecularTexture.destroy();

    vkDestroyPipeline(m_device->logicalDevice, m_cubeGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_cubePipelineLayout, nullptr);

    vkDestroyPipeline(m_device->logicalDevice, m_emissionGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_emissionPipelineLayout, nullptr);
}

void mesh::createUniformBuffers()
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
        // create directional light uniform buffer
        VkDeviceSize bufferSize = sizeof(DirectionalLightDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_directionalLightUB);
        m_directionalLightUB.setupDescriptor();
    }

    {
        // create flash light uniform buffer
        VkDeviceSize bufferSize = sizeof(FlashLightDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_flashLightUB);
        m_flashLightUB.setupDescriptor();
    }
}
void mesh::createDescriptorSets()
{
    {
        std::vector<VkDescriptorSetLayout> sceneLayouts(m_settings.max_frames, m_descriptorSetLayouts.scene);
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(sceneLayouts.size()),
            .pSetLayouts = sceneLayouts.data(),
        };
        m_perFrameDescriptorSets.resize(m_settings.max_frames);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, m_perFrameDescriptorSets.data()));

        for (size_t frameIdx = 0; frameIdx < m_settings.max_frames; frameIdx++) {
            std::vector<VkDescriptorBufferInfo> bufferInfos{
                m_mvpUBs[frameIdx].descriptorInfo,
                m_sceneUB.descriptorInfo,
                m_pointLightUB.descriptorInfo,
                m_directionalLightUB.descriptorInfo,
                m_flashLightUB.descriptorInfo,
            };

            std::vector<VkWriteDescriptorSet> descriptorWrites;

            for(auto & bufferInfo : bufferInfos){
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_perFrameDescriptorSets[frameIdx],
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

    {
        std::array<VkDescriptorSetLayout, 1> materialLayouts{ m_descriptorSetLayouts.material };

        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(materialLayouts.size()),
            .pSetLayouts = materialLayouts.data(),
        };

        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &m_cubeMaterialDescriptorSets));

        std::array<VkWriteDescriptorSet, 2> descriptorWrites;

        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_cubeMaterialDescriptorSets,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &m_containerDiffuseTexture.descriptorInfo,
            .pTexelBufferView = nullptr, // Optional
        };

        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_cubeMaterialDescriptorSets,
            .dstBinding = 1,
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
void mesh::createDescriptorSetLayout()
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
        VkDescriptorSetLayoutBinding directionalLightLayoutBinding{
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding flashLightLayoutBinding{
            .binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };

        std::array<VkDescriptorSetLayoutBinding, 5> perSceneBindings = { cameraLayoutBinding, sceneLayoutBinding,
                                                                         pointLightLayoutBinding, directionalLightLayoutBinding, flashLightLayoutBinding };
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
        VkDescriptorSetLayoutBinding samplerContainerDiffuseLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding samplerContainerSpecularLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };

        std::array<VkDescriptorSetLayoutBinding, 2> perMaterialBindings = { samplerContainerDiffuseLayoutBinding,
                                                                            samplerContainerSpecularLayoutBinding };
        VkDescriptorSetLayoutCreateInfo perMaterialLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(perMaterialBindings.size()),
            .pBindings = perMaterialBindings.data(),
        };

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->logicalDevice, &perMaterialLayoutInfo, nullptr,
                                                    &m_descriptorSetLayouts.material));
    }
}
void mesh::createGraphicsPipeline()
{
    vkl::PipelineBuilder pipelineBuilder;
    vkl::VertexLayout::setPipelineVertexInputState({vkl::VertexComponent::POSITION, vkl::VertexComponent::NORMAL, vkl::VertexComponent::UV, vkl::VertexComponent::COLOR});
    pipelineBuilder._vertexInputInfo = vkl::VertexLayout::_pipelineVertexInputStateCreateInfo;
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
        auto vertShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "model_loading/mesh/cube.vert.spv");
        auto fragShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "model_loading/mesh/cube.frag.spv");
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
        auto vertShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "model_loading/mesh/emission.vert.spv");
        auto fragShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "model_loading/mesh/emission.frag.spv");
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
void mesh::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0] = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(m_settings.max_frames * 5),
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

void mesh::updateUniformBuffer(uint32_t currentFrameIndex)
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

        FlashLightDataLayout flashLightData{
            .position = glm::vec4(m_camera.m_position, 1.0f),
            .direction = glm::vec4(m_camera.m_front, 1.0f),
            .ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
            .diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
            .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .cutOff = glm::cos(glm::radians(12.5f)),
            .outerCutOff = glm::cos(glm::radians(17.5f)),
        };

        m_flashLightUB.map();
        m_flashLightUB.copyTo(&flashLightData, sizeof(FlashLightDataLayout));
        m_flashLightUB.unmap();
    }

    {
        m_pointLightUB.map();
        m_pointLightUB.copyTo(&pointLightData, sizeof(PointLightDataLayout));
        m_pointLightUB.unmap();
    }

    {
        m_directionalLightUB.map();
        m_directionalLightUB.copyTo(&directionalLightData, sizeof(DirectionalLightDataLayout));
        m_directionalLightUB.unmap();
    }
}

void mesh::recordCommandBuffer()
{
    VkCommandBuffer commandBuffer = m_commandBuffers[m_imageIdx];
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
        .framebuffer = m_framebuffers[m_imageIdx],
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

    VkBuffer vertexBuffers[] = { m_cubeMesh.getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_cubeMesh.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 2> descriptorSets{ m_perFrameDescriptorSets[m_currentFrame],
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

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_cubeMesh.getIndicesCount()), 1, 0, 0, 0);
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
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_cubeMesh.getIndicesCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}
void mesh::createTextures()
{
    {
        loadImageFromFile(m_containerDiffuseTexture, (textureDir / "container2.png").u8string().c_str());
        m_containerDiffuseTexture.view = m_device->createImageView(m_containerDiffuseTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerDiffuseTexture.sampler));
        m_containerDiffuseTexture.setupDescriptor(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    }

    {
        loadImageFromFile(m_containerSpecularTexture, (textureDir / "container2_specular.png").u8string().c_str());
        m_containerSpecularTexture.view = m_device->createImageView(m_containerSpecularTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerSpecularTexture.sampler));
        m_containerSpecularTexture.setupDescriptor(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    }
}
void mesh::createPipelineLayout()
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

void mesh::setupDescriptors()
{
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayout();
}
void mesh::initDerive()
{
    loadMeshes();
    createUniformBuffers();
    createTextures();
    setupDescriptors();
    createSyncObjects();
    createGraphicsPipeline();
}

void mesh::loadMeshes()
{
    m_cubeMesh.setup(m_device, m_queues.graphics, cubeVertices);
}

int main()
{
    mesh app;

    app.init();
    app.run();
    app.finish();
}
