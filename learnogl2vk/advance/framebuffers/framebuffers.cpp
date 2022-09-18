#include "framebuffers.h"

// per scene data
// general scene data
struct SceneDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 viewPosition;
};

// vertex data
const std::vector<vkl::VertexLayout> quadVertices = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
    // positions   // texCoords
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f},  {1.0f, 0.0f}},

    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f},  {1.0f, 0.0f}},
    {{1.0f,  1.0f},  {1.0f, 1.0f}}
};

std::vector<vkl::VertexLayout> planeVertices{
    {{5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},

    {{5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},
    {{5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},
};

std::vector<vkl::VertexLayout> cubeVertices = {{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
                                               {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
                                               {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
                                               {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

                                               {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                                               {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                               {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                                               {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

                                               {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                                               {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                               {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

                                               {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                                               {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

                                               {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
                                               {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
                                               {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

                                               {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                                               {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};

void framebuffers::drawFrame() {
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();
    vkl::vklBase::submitFrame();
}

void framebuffers::getEnabledFeatures() {
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}

void framebuffers::updateUniformBuffer() {
    SceneDataLayout sceneData{
        .view         = m_camera.GetViewMatrix(),
        .proj         = m_camera.GetProjectionMatrix(),
        .viewProj     = m_camera.GetViewProjectionMatrix(),
        .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
    };
    m_sceneUBO.update(&sceneData);
}

void framebuffers::initDerive() {
    loadScene();
    prepareOffscreen();
    setupShaders();
    buildCommands();
}

void framebuffers::loadScene() {
    { m_sceneUBO.setupBuffer(m_device, sizeof(SceneDataLayout)); }

    {
        m_quadMesh.setupMesh(m_device, m_queues.transfer, quadVertices);
    }

    {
        m_cubeMesh.setupMesh(m_device, m_queues.transfer, cubeVertices);
        m_cubeMesh.pushImage(textureDir / "container.jpg", m_queues.transfer);

        m_planeMesh.setupMesh(m_device, m_queues.transfer, planeVertices);
        m_planeMesh.pushImage(textureDir / "metal.png", m_queues.transfer);
    }

    {
        m_defaultScene.pushCamera(&m_camera, &m_sceneUBO)
            .pushMeshObject(&m_planeMesh, &m_offscreenPass.shaderPass)
            .pushMeshObject(&m_cubeMesh, &m_offscreenPass.shaderPass, glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, -1.0f)))
            .pushMeshObject(&m_cubeMesh, &m_offscreenPass.shaderPass, glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)));
    }

    m_deletionQueue.push_function([&]() {
        m_quadMesh.destroy();
        m_planeMesh.destroy();
        m_cubeMesh.destroy();
        m_sceneUBO.destroy();
    });
}

void framebuffers::setupShaders() {
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> globalBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };

    auto shaderDir = glslShaderDir / m_sessionName;

    // build Shader
    {
        m_offscreenPass.shaderEffect.pushSetLayout(m_device->logicalDevice, globalBindings)
            .pushSetLayout(m_device->logicalDevice, materialBindings)
            .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "shader.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "shader.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
            .buildPipelineLayout(m_device->logicalDevice);

        m_offscreenPass.build(m_device, m_pipelineBuilder);
    }

    {
        m_pipelineBuilder._depthStencil = vkl::init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);
        m_postProcessPass.shaderEffect.pushSetLayout(m_device->logicalDevice, materialBindings)
            .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "post_process.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "post_process.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
            .buildPipelineLayout(m_device->logicalDevice);

        m_postProcessPass.build(m_device, m_defaultRenderPass, m_pipelineBuilder);
    }

    {
        m_defaultScene.setupDescriptor(m_device->logicalDevice);
    }

    {
        m_postProcessPass.descriptorSets.resize(m_swapChainImageViews.size());
        std::vector<VkDescriptorPoolSize> sizes{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_postProcessPass.descriptorSets.size())}
        };
        VkDescriptorPoolCreateInfo createInfo = vkl::init::descriptorPoolCreateInfo(sizes, m_postProcessPass.descriptorSets.size());
        vkCreateDescriptorPool(m_device->logicalDevice, &createInfo, nullptr, &m_postProcessPass.descriptorPool);

        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_postProcessPass.descriptorPool,
                                                                                     m_postProcessPass.shaderEffect.setLayouts.data(),
                                                                                     1);
        for (auto &set : m_postProcessPass.descriptorSets){
            VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &set));
        }

        for (auto i = 0; i < m_postProcessPass.descriptorSets.size(); i++){
            VkWriteDescriptorSet writeDescriptorSet = vkl::init::writeDescriptorSet(m_postProcessPass.descriptorSets[i],
                                                                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                                    0, &m_offscreenPass.colorAttachments[i].descriptorInfo);
            vkUpdateDescriptorSets(m_device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
        }
    }

    m_deletionQueue.push_function([&]() {
        m_postProcessPass.destroy(m_device);
        m_offscreenPass.destroy(m_device);
        m_shaderCache.destory(m_device->logicalDevice);
    });
}

void framebuffers::prepareOffscreen() {
    m_offscreenPass.prepareAttachmentResources(m_device, m_queues.transfer, m_swapChainImageViews.size(), m_swapChainExtent);
    m_offscreenPass.prepareRenderPass(m_device, m_swapChainImageFormat);
    m_offscreenPass.prepareFramebuffers(m_device, m_swapChainImages.size(), m_swapChainExtent);
}

void framebuffers::buildCommands() {
    for (uint32_t frameIdx = 0; frameIdx < m_commandBuffers.size(); frameIdx++) {
        auto &commandBuffer = m_commandBuffers[frameIdx];

        VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

        // dynamic state
        const VkViewport viewport =
            vkl::init::viewport(static_cast<float>(m_windowData.width), static_cast<float>(m_windowData.height));
        const VkRect2D scissor = vkl::init::rect2D(m_swapChainExtent);

        // record command
        vkResetCommandBuffer(commandBuffer, 0);
        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // default render pass
        {
            std::vector<VkClearValue> clearValues(2);
            clearValues[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};
            VkRenderPassBeginInfo renderPassInfo =
                vkl::init::renderPassBeginInfo(m_offscreenPass.renderPass, clearValues, m_offscreenPass.framebuffers[frameIdx]);
            renderPassInfo.renderArea = vkl::init::rect2D(m_swapChainExtent);
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            m_defaultScene.draw(commandBuffer);

            vkCmdEndRenderPass(commandBuffer);
        }

        // post processing render pass
        {
            std::vector<VkClearValue> clearValues(2);
            clearValues[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};
            VkRenderPassBeginInfo renderPassInfo = vkl::init::renderPassBeginInfo(m_defaultRenderPass, clearValues, m_framebuffers[frameIdx]);
            renderPassInfo.renderArea = vkl::init::rect2D(m_swapChainExtent);
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessPass.shaderPass.layout, 0, 1, &m_postProcessPass.descriptorSets[frameIdx], 0, nullptr);
            m_quadMesh.draw(commandBuffer, &m_postProcessPass.shaderPass);

            vkCmdEndRenderPass(commandBuffer);
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    }
}

int main() {
    framebuffers app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
