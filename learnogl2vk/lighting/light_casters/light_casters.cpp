#include "light_casters.h"
#include <cstring>

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
};

MaterialDataLayout materialData{
    .shininess = 128.0f,
};

void light_casters::drawFrame()
{
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    vkWaitForFences(m_device->logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device->logicalDevice, m_swapChain, UINT64_MAX,
                                            m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
    }

    vkResetFences(m_device->logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

    updateUniformBuffer(m_currentFrame);

    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

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

void light_casters::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}
void light_casters::keyboardHandleDerive()
{
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera.move(Camera_Movement::FORWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera.move(Camera_Movement::BACKWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera.move(Camera_Movement::LEFT, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera.move(Camera_Movement::RIGHT, deltaTime);
}
void light_casters::mouseHandleDerive(int xposIn, int yposIn)
{
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    m_camera.ProcessMouseMovement(xoffset, yoffset);
}
void light_casters::cleanupDerive()
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
    m_directionalLightUB.destroy();
    m_pointLightUB.destroy();

    m_containerDiffuseTexture.cleanup(m_device->logicalDevice);
    m_containerSpecularTexture.cleanup(m_device->logicalDevice);

    // perframe sync objects
    for (size_t i = 0; i < m_settings.max_frames; i++) {
        vkDestroySemaphore(m_device->logicalDevice, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device->logicalDevice, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_device->logicalDevice, m_inFlightFences[i], nullptr);
    }

    vkDestroyPipeline(m_device->logicalDevice, m_cubeGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_cubePipelineLayout, nullptr);

    vkDestroyPipeline(m_device->logicalDevice, m_emissionGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device->logicalDevice, m_emissionPipelineLayout, nullptr);
}
void light_casters::createVertexBuffers()
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

    m_device->copyBuffer(m_graphicsQueue, stagingBuffer.buffer, m_cubeVB.buffer, bufferSize);

    stagingBuffer.destroy();
}
void light_casters::createUniformBuffers()
{
    {
        VkDeviceSize bufferSize = sizeof(CameraDataLayout);
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

    {
        // create scene uniform buffer
        VkDeviceSize bufferSize = sizeof(SceneDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_sceneUB);
        m_sceneUB.descriptorInfo = {
            .buffer = m_sceneUB.buffer,
            .offset = 0,
            .range = bufferSize,
        };
    }

    {
        // create point light uniform buffer
        VkDeviceSize bufferSize = sizeof(PointLightDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_pointLightUB);
        m_pointLightUB.descriptorInfo = {
            .buffer = m_pointLightUB.buffer,
            .offset = 0,
            .range = bufferSize,
        };
    }

    {
        // create directional light uniform buffer
        VkDeviceSize bufferSize = sizeof(DirectionalLightDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_directionalLightUB);
        m_directionalLightUB.descriptorInfo = {
            .buffer = m_directionalLightUB.buffer,
            .offset = 0,
            .range = bufferSize,
        };
    }

    {
        // create material uniform buffer
        VkDeviceSize bufferSize = sizeof(MaterialDataLayout);
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_materialUB);
        m_materialUB.descriptorInfo = {
            .buffer = m_materialUB.buffer,
            .offset = 0,
            .range = bufferSize,
        };
    }
}
void light_casters::createDescriptorSets()
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

        for (size_t i = 0; i < m_settings.max_frames; i++) {
            std::array<VkWriteDescriptorSet, 4> descriptorWrites;

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
            descriptorWrites[3] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_perFrameDescriptorSets[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &m_directionalLightUB.descriptorInfo,
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
void light_casters::createDescriptorSetLayout()
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

        std::array<VkDescriptorSetLayoutBinding, 4> perSceneBindings = { cameraLayoutBinding, sceneLayoutBinding,
                                                                         pointLightLayoutBinding, directionalLightLayoutBinding };
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
        VkDescriptorSetLayoutBinding samplerContainerDiffuseLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutBinding samplerContainerSpecularLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };

        std::array<VkDescriptorSetLayoutBinding, 3> perMaterialBindings = { materialLayoutBinding,
                                                                            samplerContainerDiffuseLayoutBinding,
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
void light_casters::createGraphicsPipeline()
{
    vkl::utils::PipelineBuilder pipelineBuilder;
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
        auto vertShaderCode = vkl::utils::readFile(glslShaderDir / "lighting/light_casters/cube.vert.spv");
        auto fragShaderCode = vkl::utils::readFile(glslShaderDir / "lighting/light_casters/cube.frag.spv");
        VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
        pipelineBuilder._pipelineLayout = m_cubePipelineLayout;
        m_cubeGraphicsPipeline = pipelineBuilder.buildPipeline(m_device->logicalDevice, m_renderPass);
        vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
    }

    pipelineBuilder._shaderStages.clear();

    {
        auto vertShaderCode = vkl::utils::readFile(glslShaderDir / "lighting/light_casters/emission.vert.spv");
        auto fragShaderCode = vkl::utils::readFile(glslShaderDir / "lighting/light_casters/emission.frag.spv");
        VkShaderModule vertShaderModule = m_device->createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = m_device->createShaderModule(fragShaderCode);
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
        pipelineBuilder._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
        pipelineBuilder._pipelineLayout = m_emissionPipelineLayout;
        m_emissionGraphicsPipeline = pipelineBuilder.buildPipeline(m_device->logicalDevice, m_renderPass);
        vkDestroyShaderModule(m_device->logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->logicalDevice, vertShaderModule, nullptr);
    }
}
void light_casters::createSyncObjects()
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
void light_casters::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0] = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(m_settings.max_frames * 4 + 1),
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
void light_casters::updateUniformBuffer(uint32_t currentFrameIndex)
{
    {
        CameraDataLayout cameraData{
            .view = m_camera.GetViewMatrix(),
            .proj = glm::perspective(m_camera.Zoom, m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.01f,
                                     100.0f),
        };
        cameraData.proj[1][1] *= -1;
        cameraData.viewProj = cameraData.proj * cameraData.view;
        m_mvpUBs[currentFrameIndex].map();
        m_mvpUBs[currentFrameIndex].copyTo(&cameraData, sizeof(CameraDataLayout));
        m_mvpUBs[currentFrameIndex].unmap();
    }

    {
        SceneDataLayout sceneData{
            .viewPosition = glm::vec4(m_camera.Position, 1.0f),
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
        m_directionalLightUB.map();
        m_directionalLightUB.copyTo(&directionalLightData, sizeof(DirectionalLightDataLayout));
        m_directionalLightUB.unmap();
    }

    {
        m_materialUB.map();
        m_materialUB.copyTo(&materialData, sizeof(MaterialDataLayout));
        m_materialUB.unmap();
    }
}
void light_casters::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
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
void light_casters::createTextures()
{
    loadImageFromFile(m_containerDiffuseTexture.image, m_containerDiffuseTexture.memory,
                      (textureDir / "container2.png").u8string().c_str());
    loadImageFromFile(m_containerSpecularTexture.image, m_containerSpecularTexture.memory,
                      (textureDir / "container2_specular.png").u8string().c_str());

    m_containerDiffuseTexture.imageView = m_device->createImageView(m_containerDiffuseTexture.image, VK_FORMAT_R8G8B8A8_SRGB);
    m_containerSpecularTexture.imageView = m_device->createImageView(m_containerSpecularTexture.image, VK_FORMAT_R8G8B8A8_SRGB);

    VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = m_device->properties.limits.maxSamplerAnisotropy;
    VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerDiffuseTexture.sampler));
    VK_CHECK_RESULT(vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_containerSpecularTexture.sampler));

    m_containerDiffuseTexture.descriptorInfo = {
        .sampler = m_containerDiffuseTexture.sampler,
        .imageView = m_containerDiffuseTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    m_containerSpecularTexture.descriptorInfo = {
        .sampler = m_containerSpecularTexture.sampler,
        .imageView = m_containerSpecularTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
}
void light_casters::createPipelineLayout()
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

void light_casters::setupDescriptors()
{
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayout();
}
void light_casters::initDerive()
{
    createVertexBuffers();
    createUniformBuffers();
    createTextures();
    setupDescriptors();
    createSyncObjects();
    createGraphicsPipeline();
}
light_casters::light_casters()
{
    m_width = 2400;
    m_height = 1800;
}

int main()
{
    light_casters app;

    app.init();
    app.run();
    app.finish();
}
