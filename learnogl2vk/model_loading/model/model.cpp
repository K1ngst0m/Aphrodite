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
    vkl::vklBase::prepareFrame();
    updateUniformBuffer(m_currentFrame);
    recordCommandBuffer(m_currentFrame);
    vkl::vklBase::submitFrame();
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

    m_planeMesh.destroy();
    m_model.destroy();
    sceneUB.destroy();
    pointLightUB.destroy();
    directionalLightUB.destroy();

    m_modelShaderEffect.destroy(m_device->logicalDevice);
    m_modelShaderPass.destroy(m_device->logicalDevice);
    m_planeShaderEffect.destroy(m_device->logicalDevice);
    m_planeShaderPass.destroy(m_device->logicalDevice);
    m_shaderCache.destory(m_device->logicalDevice);
}

void model::createDescriptorPool()
{
    uint32_t maxSetSize = m_settings.max_frames;
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_settings.max_frames * 3)},
    };

    {
        std::vector<VkDescriptorPoolSize> setInfos = m_model.getDescriptorSetInfo();
        for (auto& setInfo : setInfos){
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    {
        std::vector<VkDescriptorPoolSize> setInfos = m_planeMesh.getDescriptorSetInfo();
        for (auto& setInfo : setInfos){
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);

    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));
}

void model::updateUniformBuffer(uint32_t frameIdx)
{
    {
        SceneDataLayout sceneData{
            .view = m_camera.GetViewMatrix(),
            .proj = m_camera.GetProjectionMatrix(),
            .viewProj = m_camera.GetViewProjectionMatrix(),
            .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
        };
        sceneUB.map();
        sceneUB.copyTo(&sceneData, sizeof(SceneDataLayout));
        sceneUB.unmap();
    }
}

void model::recordCommandBuffer(uint32_t frameIdx)
{
    auto & commandBuffer = m_commandBuffers[frameIdx];
    auto & imageIndex = m_imageIdx;

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // render pass
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassInfo = vkl::init::renderPassBeginInfo(m_defaultRenderPass, clearValues, m_framebuffers[imageIndex]);
    renderPassInfo.renderArea = {
        .offset = { 0, 0 },
        .extent = m_swapChainExtent,
    };

    // dynamic state
    const VkViewport viewport = vkl::init::viewport(static_cast<float>(m_windowData.width), static_cast<float>(m_windowData.height));
    const VkRect2D scissor = vkl::init::rect2D(m_swapChainExtent);

    // vertex buffer
    VkBuffer vertexBuffers[] = { m_model.getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };

    // descriptor sets
    std::vector<VkDescriptorSet> descriptorSets{ m_globalDescriptorSet[frameIdx] };

    // record command
    vkResetCommandBuffer(commandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelShaderEffect.builtLayout, 0,
                            static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    // scene drawing
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelShaderPass.builtPipeline);
        glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        modelTransform = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        m_model.draw(commandBuffer, &m_modelShaderPass, modelTransform);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_planeShaderPass.builtPipeline);
        glm::mat4 planeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f));
        m_planeMesh.draw(commandBuffer, &m_planeShaderPass, planeTransform);
    }

    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void model::initDerive()
{
    loadScene();
    setupShaders();
    createDescriptorPool();
    setupDescriptorSets();
}

void model::loadScene()
{
    // create scene uniform buffer
    {
        m_device->createBuffer(sizeof(SceneDataLayout), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sceneUB);
        sceneUB.setupDescriptor();
    }

    // create point light uniform buffer
    {
        m_device->createBuffer(sizeof(PointLightDataLayout), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pointLightUB);
        pointLightUB.setupDescriptor();
        pointLightUB.map();
        pointLightUB.copyTo(&pointLightData, sizeof(PointLightDataLayout));
        pointLightUB.unmap();
    }

    // create directional light uniform buffer
    {
        m_device->createBuffer(sizeof(DirectionalLightDataLayout), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, directionalLightUB);
        directionalLightUB.setupDescriptor();
        directionalLightUB.map();
        directionalLightUB.copyTo(&directionalLightData, sizeof(DirectionalLightDataLayout));
        directionalLightUB.unmap();
    }

    // load model data
    {
        m_model.loadFromFile(m_device, m_queues.transfer, modelDir/"FlightHelmet/glTF/FlightHelmet.gltf");
    }

    // load plane data
    {
        std::vector<vkl::VertexLayout> planeVertices {
            // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
            {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
            {{-5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
            {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},

            {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
            {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
            {{ 5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
        };

        m_planeMesh.setupMesh(m_device, m_queues.transfer, planeVertices);
        m_planeMesh.pushImage(textureDir/"metal.png", m_queues.transfer);
    }

}

void model::setupShaders() {
    // per-scene layout
    std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };

    // per-material layout
    std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
        vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };

    // build Shader
    auto shaderDir = glslShaderDir/m_sessionName;
    {
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings)
                           .pushSetLayout(m_device->logicalDevice, perMaterialBindings)
                           .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"model.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"model.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                           .buildPipelineLayout(m_device->logicalDevice);
        m_modelShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_modelShaderEffect);
    }

    {
        m_planeShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings)
                           .pushSetLayout(m_device->logicalDevice, perMaterialBindings)
                           .pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0))
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"plane.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                           .pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"plane.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                           .buildPipelineLayout(m_device->logicalDevice);
        m_planeShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_planeShaderEffect);
    }
}

void model::setupDescriptorSets()
{
    auto &sceneSetLayout = m_modelShaderEffect.setLayouts[DESCRIPTOR_SET_SCENE];
    auto &materialSetLayout = m_modelShaderEffect.setLayouts[DESCRIPTOR_SET_MATERIAL];

    m_globalDescriptorSet.resize(m_settings.max_frames);
    // scene
    {
        for (auto & frameIdx : m_globalDescriptorSet) {
            std::vector<VkDescriptorSetLayout> sceneLayouts(1, sceneSetLayout);
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_descriptorPool, &sceneSetLayout, 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->logicalDevice, &allocInfo, &frameIdx));

            std::vector<VkDescriptorBufferInfo> bufferInfos{
                sceneUB.descriptorInfo,
                pointLightUB.descriptorInfo,
                directionalLightUB.descriptorInfo,
            };
            std::vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto &bufferInfo : bufferInfos) {
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frameIdx,
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
        m_model.setupDescriptor(materialSetLayout, m_descriptorPool);
        m_planeMesh.setupDescriptor(materialSetLayout, m_descriptorPool);
    }
}

int main()
{
    model app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}
