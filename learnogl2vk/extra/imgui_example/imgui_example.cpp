#include "imgui_example.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

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

std::vector<vkl::VertexLayout> planeVertices {
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
    {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},

    {{ 5.0f, -0.5f,  5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f},{1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
    {{ 5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f},{1.0f, 1.0f, 1.0f}},
};

void imgui_example::drawFrame()
{
    vkl::vklBase::prepareFrame();
    updateUniformBuffer();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
    vklBase::recordCommandBuffer([&](VkCommandBuffer commandBuffer){
        // m_defaultScene.drawScene(commandBuffer);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }, m_imageIdx);
    vkl::vklBase::submitFrame();
}

void imgui_example::getEnabledFeatures()
{
    assert(m_device->features.samplerAnisotropy);
    m_device->enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
}

void imgui_example::updateUniformBuffer()
{
    SceneDataLayout sceneData{
        .view = m_camera.GetViewMatrix(),
        .proj = m_camera.GetProjectionMatrix(),
        .viewProj = m_camera.GetViewProjectionMatrix(),
        .viewPosition = glm::vec4(m_camera.m_position, 1.0f),
    };
    sceneUBO.update(&sceneData);
}

void imgui_example::initDerive()
{
    loadScene();
    setupShaders();
    setupImGui();
}

void imgui_example::loadScene()
{
    {
        sceneUBO.setupBuffer(m_device, sizeof(SceneDataLayout));
        pointLightUBO.setupBuffer(m_device, sizeof(PointLightDataLayout), &pointLightData);
        directionalLightUBO.setupBuffer(m_device, sizeof(DirectionalLightDataLayout), &directionalLightData);
    }

    {
        m_model.loadFromFile(m_device, m_queues.transfer, modelDir/"FlightHelmet/glTF/FlightHelmet.gltf");
        m_planeMesh.setupMesh(m_device, m_queues.transfer, planeVertices);
        m_planeMesh.pushImage(textureDir/"metal.png", m_queues.transfer);
    }

    {
        m_defaultScene.pushUniform(&sceneUBO);
        m_defaultScene.pushUniform(&pointLightUBO);
        m_defaultScene.pushUniform(&directionalLightUBO);

        glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        modelTransform = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        m_defaultScene.pushObject(&m_model, &m_modelShaderPass, modelTransform);

        glm::mat4 planeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.4f, 0.0f));
        m_defaultScene.pushObject(&m_planeMesh, &m_planeShaderPass, planeTransform);
    }

    m_deletionQueue.push_function([&](){
        m_planeMesh.destroy();
        m_model.destroy();
        directionalLightUBO.destroy();
        pointLightUBO.destroy();
        sceneUBO.destroy();
    });
}

void imgui_example::setupImGui() {
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000,
        .poolSizeCount = std::size(poolSizes),
        .pPoolSizes    = poolSizes,
    };

    VkDescriptorPool imguiPool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(m_window, true);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance       = m_instance,
        .PhysicalDevice = m_device->physicalDevice,
        .Device         = m_device->logicalDevice,
        .Queue          = m_queues.graphics,
        .DescriptorPool = imguiPool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };

    ImGui_ImplVulkan_Init(&initInfo, m_defaultRenderPass);

    // execute a gpu command to upload imgui font textures
    immediateSubmit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    glfwSetKeyCallback(m_window, ImGui_ImplGlfw_KeyCallback);
    glfwSetMouseButtonCallback(m_window, ImGui_ImplGlfw_MouseButtonCallback);

    m_deletionQueue.push_function([=](){
        vkDestroyDescriptorPool(m_device->logicalDevice, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}

void imgui_example::setupShaders()
{
    // per-scene layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perSceneBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        };
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings);
        m_planeShaderEffect.pushSetLayout(m_device->logicalDevice, perSceneBindings);
    }

    // per-material layout
    {
        std::vector<VkDescriptorSetLayoutBinding> perMaterialBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        m_modelShaderEffect.pushSetLayout(m_device->logicalDevice, perMaterialBindings);
        m_planeShaderEffect.pushSetLayout(m_device->logicalDevice, perMaterialBindings);
    }

    // push constants
    {
        m_modelShaderEffect.pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        m_planeShaderEffect.pushConstantRanges(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
    }

    // build Shader
    {
        auto shaderDir = glslShaderDir/m_sessionName;
        m_modelShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"model.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
        m_modelShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"model.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
        m_modelShaderEffect.buildPipelineLayout(m_device->logicalDevice);
        m_modelShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_modelShaderEffect);

        m_planeShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"plane.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
        m_planeShaderEffect.pushShaderStages(m_shaderCache.getShaders(m_device, shaderDir/"plane.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
        m_planeShaderEffect.buildPipelineLayout(m_device->logicalDevice);
        m_planeShaderPass.build(m_device->logicalDevice, m_defaultRenderPass, m_pipelineBuilder, &m_planeShaderEffect);
    }

    m_defaultScene.setupDescriptor(m_device->logicalDevice);

    m_deletionQueue.push_function([&](){
        m_defaultScene.destroy(m_device->logicalDevice);
        m_modelShaderEffect.destroy(m_device->logicalDevice);
        m_modelShaderPass.destroy(m_device->logicalDevice);
        m_planeShaderEffect.destroy(m_device->logicalDevice);
        m_planeShaderPass.destroy(m_device->logicalDevice);
        m_shaderCache.destory(m_device->logicalDevice);
    });
}

int main()
{
    imgui_example app;

    app.vkl::vklBase::init();
    app.vkl::vklBase::run();
    app.vkl::vklBase::finish();
}

