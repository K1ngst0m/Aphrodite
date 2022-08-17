#include "vklBase.h"

#include <cstring>
#include <iostream>

/*
** - https://learnopengl.com/Getting-started/Textures
 */

class textures : public vkl::vkBase {
public:
    textures()
    {
        m_width = 800;
        m_height = 600;
    }
    ~textures() override = default;

    // triangle data
private:
    // mvp matrix data layout
    struct MVPUBOLayout {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    // vertex data layout
    struct VertexLayout {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(VertexLayout);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(VertexLayout, pos),
            };

            attributeDescriptions[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(VertexLayout, color),
            };

            attributeDescriptions[2] = {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(VertexLayout, texCoord),
            };

            return attributeDescriptions;
        }
    };

    // vertex data
    const std::vector<VertexLayout> quadVerticesData = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                                         { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                                         { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                                         { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } };
    // index data
    const std::vector<uint16_t> quadIndicesData = { 0, 1, 2, 2, 3, 0 };

private:
    void initDerive() override
    {
        createVertexBuffers();
        createIndexBuffers();
        createUniformBuffers();
        createTextures();
        createDescriptorPool();
        createDescriptorSetLayout();
        createDescriptorSets();
        createSyncObjects();
        createGraphicsPipeline();
    }

    void drawFrame() override
    {
        vkWaitForFences(m_device->logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device->logicalDevice, m_swapChain, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE,
                                                &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            VK_CHECK_RESULT(result);
        }

        vkResetFences(m_device->logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

        vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
        recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

        updateUniformBuffer(m_currentFrame);

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_commandBuffers[m_currentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,
        };

        VK_CHECK_RESULT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]));

        VkSwapchainKHR swapChains[] = { m_swapChain };

        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
            .pResults = nullptr, // Optional
        };

        result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
            m_framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            VK_CHECK_RESULT(result);
        }

        m_currentFrame = (m_currentFrame + 1) % m_settings.max_frames;
    }

    // enable anisotropic filtering features
    void getEnabledFeatures() override
    {
        assert(m_device->features.samplerAnisotropy);
        m_device->features = {
            .samplerAnisotropy = VK_TRUE,
        };
    }

    void cleanupDerive() override
    {
        for (size_t i = 0; i < m_settings.max_frames; i++) {
            m_mvpUBs[i].destroy();
        }

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            vkDestroySemaphore(m_device->logicalDevice, m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device->logicalDevice, m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device->logicalDevice, m_inFlightFences[i], nullptr);
        }

        vkDestroyDescriptorPool(m_device->logicalDevice, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayout, nullptr);

        m_quadIB.destroy();
        m_quadVB.destroy();

        m_containerTexture.cleanup(m_device->logicalDevice);
        m_awesomeFaceTexture.cleanup(m_device->logicalDevice);

        vkDestroyPipeline(m_device->logicalDevice, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device->logicalDevice, m_pipelineLayout, nullptr);
    }

private:
    void createVertexBuffers()
    {
        VkDeviceSize bufferSize = sizeof(quadVerticesData[0]) * quadVerticesData.size();

        vkl::Buffer stagingBuffer;
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        stagingBuffer.map();
        stagingBuffer.copyTo(quadVerticesData.data(), static_cast<size_t>(bufferSize));
        stagingBuffer.unmap();

        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_quadVB);

        m_device->copyBuffer(m_graphicsQueue, stagingBuffer.buffer, m_quadVB.buffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createIndexBuffers()
    {
        VkDeviceSize bufferSize = sizeof(quadIndicesData[0]) * quadIndicesData.size();

        vkl::Buffer stagingBuffer;
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        stagingBuffer.map();
        stagingBuffer.copyTo(quadIndicesData.data(), static_cast<size_t>(bufferSize));
        stagingBuffer.unmap();

        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_quadIB);

        m_device->copyBuffer(m_graphicsQueue, stagingBuffer.buffer, m_quadIB.buffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(MVPUBOLayout);

        m_mvpUBs.resize(m_settings.max_frames);

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_mvpUBs[i]);
            m_mvpUBs[i].descriptorInfo = {
                .buffer = m_mvpUBs[i].buffer,
                .offset = 0,
                .range = bufferSize,
            };
        }
    }

    void createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(m_settings.max_frames, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(m_settings.max_frames),
            .pSetLayouts = layouts.data(),
        };

        m_descriptorSets.resize(m_settings.max_frames);

        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, m_descriptorSets.data()));

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            std::array<VkWriteDescriptorSet, 3> descriptorWrites;

            descriptorWrites[0] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr, // Optional
                .pBufferInfo = &m_mvpUBs[i].descriptorInfo,
                .pTexelBufferView = nullptr, // Optional
            };

            descriptorWrites[1] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &m_containerTexture.descriptorInfo,
                .pTexelBufferView = nullptr, // Optional
            };

            descriptorWrites[2] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &m_awesomeFaceTexture.descriptorInfo,
                .pTexelBufferView = nullptr, // Optional
            };

            vkUpdateDescriptorSets(m_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                                   nullptr);
        }
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
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

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerContainerLayoutBinding,
                                                                 samplerAwesomefaceLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout));
    }

    void createSyncObjects()
    {
        m_imageAvailableSemaphores.resize(m_settings.max_frames);
        m_renderFinishedSemaphores.resize(m_settings.max_frames);
        m_inFlightFences.resize(m_settings.max_frames);

        VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            VK_CHECK_RESULT(vkCreateSemaphore(m_device->logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
            VK_CHECK_RESULT(vkCreateSemaphore(m_device->logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
            VK_CHECK_RESULT(vkCreateFence(m_device->logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]));
        }
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = vkl::utils::readFile(glslShaderDir / "getting_started/textures/shader.vert.spv");
        auto fragShaderCode = vkl::utils::readFile(glslShaderDir / "getting_started/textures/shader.frag.spv");

        VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main",
        };

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main",
        };

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = VertexLayout::getBindingDescription();
        auto attributeDescriptions = VertexLayout::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data(),
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };

        VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)m_swapChainExtent.width,
            .height = (float)m_swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D scissor{
            .offset = { 0, 0 },
            .extent = m_swapChainExtent,
        };

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data(),
        };

        VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
        };

        VkPipelineRasterizationStateCreateInfo rasterizer{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f, // Optional
            .depthBiasClamp = 0.0f, // Optional
            .depthBiasSlopeFactor = 0.0f, // Optional
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f, // Optional
            .pSampleMask = nullptr, // Optional
            .alphaToCoverageEnable = VK_FALSE, // Optional
            .alphaToOneEnable = VK_FALSE, // Optional
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo colorBlending{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY, // Optional
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
        };
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1, // Optional
            .pSetLayouts = &m_descriptorSetLayout, // Optional
            .pushConstantRangeCount = 0, // Optional
            .pPushConstantRanges = nullptr, // Optional
        };

        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device->logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

        VkPipelineDepthStencilStateCreateInfo depthStencil{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {}, // Optional
            .back = {}, // Optional
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };

        VkGraphicsPipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_pipelineLayout,
            .renderPass = m_renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE, // Optional
            .basePipelineIndex = -1, // Optional
        };

        VK_CHECK_RESULT(
                vkCreateGraphicsPipelines(m_device->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));

        vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
    }

    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};

        // mvp ubo
        poolSizes[0] = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(m_settings.max_frames),
        };

        // container texture
        poolSizes[1] = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(m_settings.max_frames),
        };

        // awesome texture
        poolSizes[2] = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(m_settings.max_frames),
        };

        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = m_settings.max_frames,
            .poolSizeCount = poolSizes.size(),
            .pPoolSizes = poolSizes.data(),
        };

        VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));
    }

    void updateUniformBuffer(uint32_t currentFrameIndex)
    {
        MVPUBOLayout ubo{
            .model = glm::mat4(1.0f),
            .view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            .proj = glm::perspective(glm::radians(90.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height,
                                     0.1f, 10.0f),
        };
        ubo.proj[1][1] *= -1;

        void *data;
        vkMapMemory(m_device->logicalDevice, m_mvpUBs[currentFrameIndex].memory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(m_device->logicalDevice, m_mvpUBs[currentFrameIndex].memory);
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0, // Optional
            .pInheritanceInfo = nullptr, // Optional
        };

        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
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

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

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

        VkBuffer vertexBuffers[] = { m_quadVB.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_quadIB.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                &m_descriptorSets[m_currentFrame], 0, nullptr);

        // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(quadIndicesData.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    }

    void createTextures()
    {
        loadImageFromFile(m_containerTexture.image, m_containerTexture.memory,
                          (textureDir / "container.jpg").u8string().c_str());
        loadImageFromFile(m_awesomeFaceTexture.image, m_awesomeFaceTexture.memory,
                          (textureDir / "awesomeface.png").u8string().c_str());

        m_containerTexture.imageView = m_device->createImageView(m_containerTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
        m_awesomeFaceTexture.imageView = m_device->createImageView(m_awesomeFaceTexture.image, VK_FORMAT_R8G8B8A8_SRGB);

        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = m_device->properties.limits.maxSamplerAnisotropy;
        VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerTexture.sampler));
        VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_awesomeFaceTexture.sampler));

        m_containerTexture.descriptorInfo = {
            .sampler = m_containerTexture.sampler,
            .imageView = m_containerTexture.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        m_awesomeFaceTexture.descriptorInfo = {
            .sampler = m_awesomeFaceTexture.sampler,
            .imageView = m_awesomeFaceTexture.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

private:
    vkl::Buffer m_quadVB;

    vkl::Buffer m_quadIB;

    std::vector<vkl::Buffer> m_mvpUBs;

    Texture m_containerTexture;
    Texture m_awesomeFaceTexture;

    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
};

int main()
{
    textures app;

    app.init();
    app.run();
    app.finish();
}
