#include "vklBase.h"

#include <cstring>
#include <iostream>

class textures_example : public vkl::vkBase {
public:
    textures_example()
    {
        m_width = 800;
        m_height = 600;
    }
    ~textures_example() override = default;

    // triangle data
private:
    // mvp matrix data layout
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    // vertex data layout
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
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
                .offset = offsetof(Vertex, pos),
            };

            attributeDescriptions[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color),
            };

            attributeDescriptions[2] = {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texCoord),
            };

            return attributeDescriptions;
        }
    };

    // vertex data
    const std::vector<Vertex> vertices = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                           { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                           { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                           { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } };
    // index data
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    struct Texture {
        VkImage Image;
        VkDeviceMemory Memory;
        VkImageView ImageView;
        VkSampler Sampler;
    };

private:
    void initDerive() override
    {
        createVertexBuffers();
        createIndexBuffers();
        createUniformBuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createDescriptorPool();
        createDescriptorSetLayout();
        createDescriptorSets();
        createSyncObjects();
        createGraphicsPipeline();
    }

    void drawFrame() override
    {
        vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE,
                                                &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            VK_CHECK_RESULT(result);
        }

        vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

        vkResetCommandBuffer(commandBuffers[m_currentFrame], 0);
        recordCommandBuffer(commandBuffers[m_currentFrame], imageIndex);

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
            .pCommandBuffers = &commandBuffers[m_currentFrame],
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

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            VK_CHECK_RESULT(result);
        }

        m_currentFrame = (m_currentFrame + 1) % m_settings.max_frames;
    }

    // enable anisotropic filtering features
    void getEnabledFeatures() override
    {
        assert(m_deviceFeatures.samplerAnisotropy);
        m_deviceFeatures = {
            .samplerAnisotropy = VK_TRUE,
        };
    }

    void cleanupDerive() override
    {
        for (size_t i = 0; i < m_settings.max_frames; i++) {
            vkDestroyBuffer(m_device, uniformBuffers[i], nullptr);
            vkFreeMemory(m_device, uniformBuffersMemory[i], nullptr);
        }

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

        vkDestroySampler(m_device, m_containerTexture.Sampler, nullptr);
        vkDestroyImageView(m_device, m_containerTexture.ImageView, nullptr);
        vkDestroyImage(m_device, m_containerTexture.Image, nullptr);
        vkFreeMemory(m_device, m_containerTexture.Memory, nullptr);

        vkDestroySampler(m_device, m_awesomeFaceTexture.Sampler, nullptr);
        vkDestroyImageView(m_device, m_awesomeFaceTexture.ImageView, nullptr);
        vkDestroyImage(m_device, m_awesomeFaceTexture.Image, nullptr);
        vkFreeMemory(m_device, m_awesomeFaceTexture.Memory, nullptr);

        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }

private:
    void createVertexBuffers()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        void *data;
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffer,
                     m_vertexBufferMemory);

        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffers()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        void *data;
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(m_settings.max_frames);
        uniformBuffersMemory.resize(m_settings.max_frames);

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i],
                         uniformBuffersMemory[i]);
        }
    }

    void createTextureImage()
    {
        loadImageFromFile(m_containerTexture.Image, m_containerTexture.Memory,
                          (textureDir / "container.jpg").u8string().c_str());
        loadImageFromFile(m_awesomeFaceTexture.Image, m_awesomeFaceTexture.Memory,
                          (textureDir / "awesomeface.png").u8string().c_str());
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

        descriptorSets.resize(m_settings.max_frames);

        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, descriptorSets.data()));

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            VkDescriptorBufferInfo bufferInfo{
                .buffer = uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject),
            };

            VkDescriptorImageInfo imageContainerInfo{
                .sampler = m_containerTexture.Sampler,
                .imageView = m_containerTexture.ImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkDescriptorImageInfo imageAwesomefaceInfo{
                .sampler = m_awesomeFaceTexture.Sampler,
                .imageView = m_awesomeFaceTexture.ImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            std::array<VkWriteDescriptorSet, 3> descriptorWrites;

            descriptorWrites[0] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr, // Optional
                .pBufferInfo = &bufferInfo,
                .pTexelBufferView = nullptr, // Optional
            };

            descriptorWrites[1] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageContainerInfo,
                .pTexelBufferView = nullptr, // Optional
            };

            descriptorWrites[2] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageAwesomefaceInfo,
                .pTexelBufferView = nullptr, // Optional
            };

            vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
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

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout));
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
            VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
            VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
            VK_CHECK_RESULT(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]));
        }
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = vkl::utils::readFile(glslShaderDir / "getting_started/textures/vert.spv");
        auto fragShaderCode = vkl::utils::readFile(glslShaderDir / "getting_started/textures/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        ;
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

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

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
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

        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = nullptr, // Optional
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_pipelineLayout,
            .renderPass = m_renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE, // Optional
            .basePipelineIndex = -1, // Optional
        };

        VK_CHECK_RESULT(
                vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));

        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    }

    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0] = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(m_settings.max_frames),
        };
        poolSizes[1] = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(m_settings.max_frames),
        };
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

        VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool));
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        UniformBufferObject ubo{
            .model = glm::mat4(1.0f),
            .view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            .proj = glm::perspective(glm::radians(90.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height,
                                     0.1f, 10.0f),
        };
        ubo.proj[1][1] *= -1;

        void *data;
        vkMapMemory(m_device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(m_device, uniformBuffersMemory[currentImage]);
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0, // Optional
            .pInheritanceInfo = nullptr, // Optional
        };

        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = m_renderPass,
            .framebuffer = m_swapChainFramebuffers[imageIndex],
            .clearValueCount = 1,
            .pClearValues = &clearColor,
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

        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                &descriptorSets[m_currentFrame], 0, nullptr);

        // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    }

    void createTextureImageView()
    {
        m_containerTexture.ImageView = createImageView(m_containerTexture.Image, VK_FORMAT_R8G8B8A8_SRGB);
        m_awesomeFaceTexture.ImageView = createImageView(m_awesomeFaceTexture.Image, VK_FORMAT_R8G8B8A8_SRGB);
    }

    void createTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = m_deviceProperties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VK_CHECK_RESULT(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_containerTexture.Sampler));
        VK_CHECK_RESULT(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_awesomeFaceTexture.Sampler));
    }

private:
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    Texture m_containerTexture;
    Texture m_awesomeFaceTexture;

    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
};

int main()
{
    textures_example app;

    app.init();
    app.run();
    app.finish();
}
