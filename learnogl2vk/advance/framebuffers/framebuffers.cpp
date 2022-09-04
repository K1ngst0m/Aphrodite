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
    prepareRenderPass();
    prepareAttachmentResouces();
    prepareFramebuffers();
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
        // TODO: multi pass support
        m_defaultScene.pushCamera(&m_camera, &m_sceneUBO)
            .pushObject(&m_planeMesh, &m_offscreenPass.shaderPass)
            .pushObject(&m_cubeMesh, &m_offscreenPass.shaderPass, glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, -1.0f)))
            .pushObject(&m_cubeMesh, &m_offscreenPass.shaderPass, glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)));
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
        m_postProcessPass.shaderEffect.pushSetLayout(m_device->logicalDevice, materialBindings)
            .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "post_process.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
            .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir / "post_process.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
            .buildPipelineLayout(m_device->logicalDevice);

        m_postProcessPass.build(m_device, m_pipelineBuilder);
    }

    {
        m_defaultScene.setupDescriptor(m_device->logicalDevice);
    }

    {
        m_postProcessPass.descriptorSets.resize(m_postProcessPass.framebuffers.size());
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
        m_postProcessPass.destory(m_device);
        m_offscreenPass.destory(m_device);
        m_defaultScene.destroy(m_device->logicalDevice);
        m_shaderCache.destory(m_device->logicalDevice);
    });
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
            std::vector<VkClearValue> clearValues(1);
            clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
            VkRenderPassBeginInfo renderPassInfo =
                vkl::init::renderPassBeginInfo(m_postProcessPass.renderPass, clearValues, m_postProcessPass.framebuffers[frameIdx]);
            renderPassInfo.renderArea = vkl::init::rect2D(m_swapChainExtent);
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // TODO post processing
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessPass.shaderPass.layout, 0, 1, &m_postProcessPass.descriptorSets[frameIdx], 0, nullptr);
            m_quadMesh.draw(commandBuffer, &m_postProcessPass.shaderPass);

            vkCmdEndRenderPass(commandBuffer);
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    }
}

void framebuffers::prepareFramebuffers() {
    m_offscreenPass.framebuffers.resize(m_swapChainImageViews.size());
    m_postProcessPass.framebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        {
            std::array<VkImageView, 2> attachments = {m_offscreenPass.colorAttachments[i].view,
                                                      m_offscreenPass.depthAttachment.view};

            VkFramebufferCreateInfo framebufferInfo{
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = m_offscreenPass.renderPass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments    = attachments.data(),
                .width           = m_swapChainExtent.width,
                .height          = m_swapChainExtent.height,
                .layers          = 1,
            };

            VK_CHECK_RESULT(vkCreateFramebuffer(m_device->logicalDevice, &framebufferInfo, nullptr, &m_offscreenPass.framebuffers[i]));
        }

        {
            VkFramebufferCreateInfo framebufferInfo{
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = m_postProcessPass.renderPass,
                .attachmentCount = 1,
                .pAttachments    = &m_swapChainImageViews[i],
                .width           = m_swapChainExtent.width,
                .height          = m_swapChainExtent.height,
                .layers          = 1,
            };

            VK_CHECK_RESULT(vkCreateFramebuffer(m_device->logicalDevice, &framebufferInfo, nullptr, &m_postProcessPass.framebuffers[i]));
        }
    }
}

void framebuffers::prepareRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format         = m_swapChainImageFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment{
        .format         = m_device->findDepthFormat(),
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDependency dependency{
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    {
        VkSubpassDescription subpass{
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &colorAttachmentRef,
            .pDepthStencilAttachment = &depthAttachmentRef,
        };

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo                 renderPassInfo{
                            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                            .attachmentCount = static_cast<uint32_t>(attachments.size()),
                            .pAttachments    = attachments.data(),
                            .subpassCount    = 1,
                            .pSubpasses      = &subpass,
                            .dependencyCount = 1,
                            .pDependencies   = &dependency,
        };

        VK_CHECK_RESULT(vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_offscreenPass.renderPass));
    }

    {
        VkSubpassDescription subpass{
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &colorAttachmentRef,
            .pDepthStencilAttachment = nullptr,
        };

        VkRenderPassCreateInfo renderPassInfo{
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &colorAttachment,
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = 1,
            .pDependencies   = &dependency,
        };

        VK_CHECK_RESULT(vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_postProcessPass.renderPass));
    }

    m_deletionQueue.push_function(
        [=]() {
            vkDestroyRenderPass(m_device->logicalDevice, m_offscreenPass.renderPass, nullptr);
            vkDestroyRenderPass(m_device->logicalDevice, m_postProcessPass.renderPass, nullptr);
        });
}

void framebuffers::prepareAttachmentResouces() {
    // offscreen - color
    {
        m_offscreenPass.colorAttachments.resize(m_swapChainImageViews.size());
        for (auto & attachment: m_offscreenPass.colorAttachments){
            m_device->createImage(m_swapChainExtent.width, m_swapChainExtent.height, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                attachment);
            attachment.view = m_device->createImageView(attachment.image, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
            m_device->transitionImageLayout(m_queues.transfer, attachment.image, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
            VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &attachment.sampler));
            attachment.setupDescriptor(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
    }

    // offscreen - depth
    {
        VkFormat depthFormat = m_device->findDepthFormat();
        m_device->createImage(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            m_offscreenPass.depthAttachment);
        m_offscreenPass.depthAttachment.view = m_device->createImageView(m_offscreenPass.depthAttachment.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_device->transitionImageLayout(m_queues.transfer, m_offscreenPass.depthAttachment.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
}

int main() {
    framebuffers app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}

