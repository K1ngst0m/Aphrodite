#include "vklBase.h"

#include <cstring>
#include <iostream>

#define VKL_DYNAMIC_STATE

class first_triangle : public vkl::vklBase {
public:
    ~first_triangle() override = default;

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

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(VertexLayout);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
        {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(VertexLayout, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(VertexLayout, color);

            return attributeDescriptions;
        }
    };

    // vertex data
    const std::vector<VertexLayout> triangleVerticesData = {
        { { -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
    };

    // index data
    const std::array<uint16_t, 3> triangleIndicesData = { 0, 1, 2 };

private:
    void initDerive() override
    {
        createVertexBuffers();
        createIndexBuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSetLayout();
        createDescriptorSets();
        createSyncObjects();
        createPipelineLayout();
        createGraphicsPipeline();
    }

    void drawFrame() override
    {
        prepareFrame();
        updateUniformBuffer(m_currentFrame);
        recordCommandBuffer(m_commandBuffers[m_currentFrame], m_imageIndices[m_currentFrame]);
        submitFrame();
    }

    void cleanupDerive() override
    {
        for (size_t i = 0; i < m_settings.max_frames; i++) {
            m_mvpUBs[i].destroy();
        }

        vkDestroyDescriptorPool(m_device->logicalDevice, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayout, nullptr);

        m_triangleIB.destroy();
        m_triangleVB.destroy();

        vkDestroyPipeline(m_device->logicalDevice, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device->logicalDevice, m_pipelineLayout, nullptr);
    }

private:
    void createVertexBuffers()
    {
        VkDeviceSize bufferSize = sizeof(triangleVerticesData[0]) * triangleVerticesData.size();

        vkl::Buffer stagingBuffer;
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        stagingBuffer.map();
        stagingBuffer.copyTo(triangleVerticesData.data(), static_cast<size_t>(bufferSize));
        stagingBuffer.unmap();

        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_triangleVB);
        m_device->copyBuffer(m_queues.graphics, stagingBuffer.buffer, m_triangleVB.buffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createIndexBuffers()
    {
        VkDeviceSize bufferSize = sizeof(triangleIndicesData[0]) * triangleIndicesData.size();

        vkl::Buffer stagingBuffer;
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        stagingBuffer.map();
        stagingBuffer.copyTo(triangleIndicesData.data(), static_cast<size_t>(bufferSize));
        stagingBuffer.unmap();

        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_triangleIB);
        m_device->copyBuffer(m_queues.graphics, stagingBuffer.buffer, m_triangleIB.buffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(MVPUBOLayout);

        m_mvpUBs.resize(m_settings.max_frames);

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_mvpUBs[i]);
            m_mvpUBs[i].setupDescriptor();
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
            VkWriteDescriptorSet descriptorWrite{
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

            vkUpdateDescriptorSets(m_device->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr, // Optional
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uboLayoutBinding,
        };

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout));
    }

    void createGraphicsPipeline()
    {
        vkl::PipelineBuilder pipelineBuilder;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions{ VertexLayout::getBindingDescription() };
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = VertexLayout::getAttributeDescriptions();
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
            auto vertShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "getting_started/first_triangle/shader.vert.spv");
            auto fragShaderCode = vkl::utils::loadSpvFromFile(glslShaderDir / "getting_started/first_triangle/shader.frag.spv");
            VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
            VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);

            pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
            pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
            pipelineBuilder._pipelineLayout = m_pipelineLayout;
            m_graphicsPipeline = pipelineBuilder.buildPipeline(m_device->logicalDevice, m_defaultRenderPass);

            vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
            vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
        }
    }

    void createDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(m_settings.max_frames),
        };

        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = static_cast<uint32_t>(m_settings.max_frames),
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };

        VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        MVPUBOLayout ubo{
            .model = glm::mat4(1.0f),
            .view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            .proj = glm::perspective(glm::radians(90.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height,
                                     0.1f, 10.0f),
        };
        ubo.proj[1][1] *= -1;

        void *data;
        vkMapMemory(m_device->logicalDevice, m_mvpUBs[currentImage].memory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(m_device->logicalDevice, m_mvpUBs[currentImage].memory);
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        vkResetCommandBuffer(commandBuffer, 0);
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
            .renderPass = m_defaultRenderPass,
            .framebuffer = m_framebuffers[imageIndex],
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

        VkBuffer vertexBuffers[] = { m_triangleVB.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_triangleIB.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                &m_descriptorSets[m_currentFrame], 0, nullptr);

        // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(triangleIndicesData.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    }

    void createPipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1, // Optional
            .pSetLayouts = &m_descriptorSetLayout, // Optional
            .pushConstantRangeCount = 0, // Optional
            .pPushConstantRanges = nullptr, // Optional
        };

        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device->logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
    }

private:
    vkl::Buffer m_triangleVB;
    vkl::Buffer m_triangleIB;

    std::vector<vkl::Buffer> m_mvpUBs;

    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
};

int main()
{
    first_triangle app;

    app.init();
    app.run();
    app.finish();
}
