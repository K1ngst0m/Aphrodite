#include "renderer.h"
#include "api/vulkan/device.h"
#include "renderer/renderer.h"
#include "scene/mesh.h"

#include "api/gpuResource.h"
#include "common/assetManager.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace aph::vk
{
Renderer::Renderer(std::shared_ptr<Window> window, const RenderConfig& config) : IRenderer(std::move(window), config)
{
    // create instance
    {
        volkInitialize();

        std::vector<const char*> extensions{};
        {
            uint32_t     glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            extensions     = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if(m_config.flags & RENDER_CFG_DEBUG) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        InstanceCreateInfo instanceCreateInfo{.enabledExtensions = extensions};

        if(m_config.flags & RENDER_CFG_DEBUG)
        {
            instanceCreateInfo.enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        VK_CHECK_RESULT(Instance::Create(instanceCreateInfo, &m_pInstance));
    }

    // create device
    {
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
        };

        DeviceCreateInfo createInfo{
            .enabledExtensions = deviceExtensions,
            // TODO select physical device
            .pPhysicalDevice = m_pInstance->getPhysicalDevices(0),
        };

        VK_CHECK_RESULT(Device::Create(createInfo, &m_pDevice));

        // get 3 type queue
        m_queue.graphics = m_pDevice->getQueueByFlags(QueueType::GRAPHICS);
        m_queue.compute  = m_pDevice->getQueueByFlags(QueueType::COMPUTE);
        m_queue.transfer = m_pDevice->getQueueByFlags(QueueType::TRANSFER);
        if(!m_queue.compute) { m_queue.compute = m_queue.graphics; }
        if(!m_queue.transfer) { m_queue.transfer = m_queue.compute; }

        // check sample count support
        {
            auto limit  = createInfo.pPhysicalDevice->getProperties().limits;
            auto counts = limit.framebufferColorSampleCounts & limit.framebufferDepthSampleCounts;
            if(!(counts & m_sampleCount)) { m_sampleCount = VK_SAMPLE_COUNT_1_BIT; }
        }
    }

    // setup swapchain
    {
        VK_CHECK_RESULT(glfwCreateWindowSurface(m_pInstance->getHandle(), m_window->getHandle(), nullptr, &m_surface));
        SwapChainCreateInfo createInfo{
            .surface      = m_surface,
            .windowHandle = m_window->getHandle(),
        };
        VK_CHECK_RESULT(m_pDevice->createSwapchain(createInfo, &m_pSwapChain));
    }

    // init default resources
    if(m_config.flags & RENDER_CFG_DEFAULT_RES)
    {
        m_frameFences.resize(m_config.maxFrames);
        m_commandBuffers.resize(m_config.maxFrames);
        m_renderSemaphore.resize(m_config.maxFrames);
        m_presentSemaphore.resize(m_config.maxFrames);

        {
            m_pSyncPrimitivesPool = std::make_unique<SyncPrimitivesPool>(m_pDevice);
        }

        // command buffer
        {
            m_pDevice->allocateCommandBuffers(m_commandBuffers.size(), m_commandBuffers.data(), getGraphicsQueue());
        }

        {
            m_pSyncPrimitivesPool->acquireSemaphore(m_presentSemaphore.size(), m_presentSemaphore.data());
            m_pSyncPrimitivesPool->acquireSemaphore(m_renderSemaphore.size(), m_renderSemaphore.data());

            for(uint32_t idx = 0; idx < m_config.maxFrames; idx++)
            {
                m_pSyncPrimitivesPool->acquireFence(m_frameFences[idx]);
            }
        }

        // pipeline cache
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            VK_CHECK_RESULT(m_pDevice->getDeviceTable()->vkCreatePipelineCache(
                m_pDevice->getHandle(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
        }
    }

    // init ui
    if(m_config.flags & RENDER_CFG_UI)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // Setup Dear ImGui style
        io.FontGlobalScale = m_ui.scale;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

        // Create font texture
        {
            unsigned char*    fontData;
            int               texWidth, texHeight;
            const std::string filename = AssetManager::GetFontDir() / "Roboto-Medium.ttf";
            io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * m_ui.scale);
            io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
            VkDeviceSize uploadSize = texWidth * texHeight * 4;

            // SRS - Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(m_ui.scale);

            std::vector<uint8_t> imageData(fontData, fontData + uploadSize);
            ImageCreateInfo      creatInfo{
                     .extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
                     .usage  = VK_IMAGE_USAGE_SAMPLED_BIT,
                     .format = VK_FORMAT_R8G8B8A8_UNORM,
                     .tiling = VK_IMAGE_TILING_OPTIMAL,
            };
            m_pDevice->createDeviceLocalImage(creatInfo, &m_ui.pFontImage, imageData);
            m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](CommandBuffer* pCmd) {
                pCmd->transitionImageLayout(m_ui.pFontImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            });
        }

        // font sampler
        {
            VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo();
            samplerInfo.magFilter           = VK_FILTER_LINEAR;
            samplerInfo.minFilter           = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VK_CHECK_RESULT(m_pDevice->createSampler(samplerInfo, &m_ui.fontSampler));
        }

        // setup descriptor
        {
            std::vector<ResourcesBinding> bindings{
                {ResourceType::COMBINE_SAMPLER_IMAGE, {ShaderStage::FS}},
            };
            m_pDevice->createDescriptorSetLayout(bindings, &m_ui.pSetLayout);

            VkDescriptorImageInfo      fontDescriptor = {m_ui.fontSampler, m_ui.pFontImage->getView()->getHandle(),
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            std::vector<ResourceWrite> writes{
                {&fontDescriptor, {}},
            };

            m_ui.set = m_ui.pSetLayout->allocateSet(writes);
        }

        // setup pipeline
        {
            GraphicsPipelineCreateInfo pipelineCreateInfo{};
            auto                       shaderDir    = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "ui";
            std::vector<VkFormat>      colorFormats = {m_pSwapChain->getFormat()};
            pipelineCreateInfo.renderingCreateInfo  = VkPipelineRenderingCreateInfo{
                 .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                 .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
                 .pColorAttachmentFormats = colorFormats.data(),
                 .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
            };
            pipelineCreateInfo.depthStencil =
                init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

            pipelineCreateInfo.setLayouts = {m_ui.pSetLayout};
            pipelineCreateInfo.constants  = {{utils::VkCast(ShaderStage::VS), 0, sizeof(m_ui.pushConstBlock)}};
            pipelineCreateInfo.shaderMapList[ShaderStage::VS] = getShaders(shaderDir / "uioverlay.vert.spv");
            pipelineCreateInfo.shaderMapList[ShaderStage::FS] = getShaders(shaderDir / "uioverlay.frag.spv");

            pipelineCreateInfo.rasterizer.cullMode  = VK_CULL_MODE_NONE;
            pipelineCreateInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            VkPipelineColorBlendAttachmentState blendAttachmentState{
                .blendEnable         = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
            pipelineCreateInfo.colorBlendAttachments[0] = blendAttachmentState;
            pipelineCreateInfo.multisampling            = init::pipelineMultisampleStateCreateInfo(m_sampleCount);

            // Vertex bindings an attributes based on ImGui vertex definition
            std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                {0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX},
            };
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
                {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},   // Location 0: Position
                {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},    // Location 1: UV
                {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)},  // Location 0: Color
            };
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = init::pipelineVertexInputStateCreateInfo();
            vertexInputInfo.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertexInputBindings.size());
            vertexInputInfo.pVertexBindingDescriptions           = vertexInputBindings.data();
            vertexInputInfo.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertexInputAttributes.size());
            vertexInputInfo.pVertexAttributeDescriptions         = vertexInputAttributes.data();
            pipelineCreateInfo.vertexInputInfo                   = vertexInputInfo;

            VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_ui.pipeline));
        }
    }
}

Renderer::~Renderer()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        m_pDevice->destroyBuffer(m_ui.pVertexBuffer);
        m_pDevice->destroyBuffer(m_ui.pIndexBuffer);

        m_pDevice->destroyImage(m_ui.pFontImage);

        m_pDevice->destroySampler(m_ui.fontSampler);
        m_pDevice->destroyDescriptorSetLayout(m_ui.pSetLayout);
        m_pDevice->destroyPipeline(m_ui.pipeline);
        if(ImGui::GetCurrentContext()) { ImGui::DestroyContext(); }
    }

    // TODO
    m_pSyncPrimitivesPool.reset(nullptr);

    for(const auto& [key, shaderModule] : shaderModuleCaches)
    {
        vkDestroyShaderModule(m_pDevice->getHandle(), shaderModule->getHandle(), nullptr);
        delete shaderModule;
    }

    vkDestroyPipelineCache(m_pDevice->getHandle(), m_pipelineCache, nullptr);

    m_pDevice->destroySwapchain(m_pSwapChain);
    vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, nullptr);
    Device::Destroy(m_pDevice);
    Instance::Destroy(m_pInstance);
};

void Renderer::beginFrame()
{
    VK_CHECK_RESULT(m_pDevice->waitForFence({m_frameFences[m_frameIdx]}));
    VK_CHECK_RESULT(m_pSwapChain->acquireNextImage(m_renderSemaphore[m_frameIdx]));
    VK_CHECK_RESULT(m_pSyncPrimitivesPool->releaseFence(m_frameFences[m_frameIdx]));

    {
        m_timer = std::chrono::high_resolution_clock::now();
    }
}

void Renderer::endFrame()
{
    auto* queue = getGraphicsQueue();
    VK_CHECK_RESULT(m_pSwapChain->presentImage(queue, {m_presentSemaphore[m_frameIdx]}));

    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;

    {
        m_frameCounter++;
        auto tEnd      = std::chrono::high_resolution_clock::now();
        auto tDiff     = std::chrono::duration<double, std::milli>(tEnd - m_timer).count();
        m_frameTimer   = (float)tDiff / 1000.0f;
        float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - m_lastTimestamp).count());
        if(fpsTimer > 1000.0f)
        {
            m_lastFPS       = static_cast<uint32_t>((float)m_frameCounter * (1000.0f / fpsTimer));
            m_frameCounter  = 0;
            m_lastTimestamp = tEnd;
        }
        m_tPrevEnd = tEnd;
    }
}

ShaderModule* Renderer::getShaders(const std::filesystem::path& path)
{
    if(!shaderModuleCaches.count(path))
    {
        std::vector<char> spvCode;
        if(path.extension() == ".spv") { spvCode = utils::loadSpvFromFile(path); }
        else { spvCode = utils::loadGlslFromFile(path); }
        auto* shaderModule       = ShaderModule::Create(m_pDevice, spvCode);
        shaderModuleCaches[path] = shaderModule;
    }
    return shaderModuleCaches[path];
}

void Renderer::UI::resize(uint32_t width, uint32_t height)
{
    ImGuiIO& io    = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(width), (float)(height));
}

void Renderer::recordUIDraw(CommandBuffer* pCommandBuffer)
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        ImDrawData* imDrawData   = ImGui::GetDrawData();
        int32_t     vertexOffset = 0;
        int32_t     indexOffset  = 0;

        if((!imDrawData) || (imDrawData->CmdListsCount == 0)) { return; }

        ImGuiIO& io = ImGui::GetIO();
        pCommandBuffer->bindPipeline(m_ui.pipeline);
        pCommandBuffer->bindDescriptorSet(m_ui.pipeline, 0, 1, &m_ui.set);

        m_ui.pushConstBlock.scale     = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
        m_ui.pushConstBlock.translate = glm::vec2(-1.0f);
        pCommandBuffer->pushConstants(m_ui.pipeline, {ShaderStage::VS}, 0, sizeof(m_ui.pushConstBlock),
                                      &m_ui.pushConstBlock);

        pCommandBuffer->bindVertexBuffers(0, 1, m_ui.pVertexBuffer, {0});
        pCommandBuffer->bindIndexBuffers(m_ui.pIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

        for(int32_t i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for(int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D         scissorRect;
                scissorRect.offset.x      = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissorRect.offset.y      = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissorRect.extent.width  = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                pCommandBuffer->setSissor(scissorRect);
                pCommandBuffer->drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

bool Renderer::updateUIDrawData(float deltaTime)
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2((float)m_window->getWidth(), (float)m_window->getHeight());
        io.DeltaTime   = deltaTime;

        ImDrawData* imDrawData       = ImGui::GetDrawData();
        bool        updateCmdBuffers = false;

        if(!imDrawData) { return false; };

        // Note: Alignment is done inside buffer creation
        VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
        VkDeviceSize indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        // Update buffers only if vertex or index count has been changed compared to current buffer size
        if((vertexBufferSize == 0) || (indexBufferSize == 0)) { return false; }

        // Vertex buffer
        if(m_ui.pVertexBuffer == nullptr || (m_ui.vertexCount != imDrawData->TotalVtxCount))
        {
            BufferCreateInfo createInfo = {.size   = static_cast<uint32_t>(vertexBufferSize),
                                           .usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           .domain = BufferDomain::Host};
            if(m_ui.pVertexBuffer)
            {
                m_pDevice->waitIdle();
                m_pDevice->unMapMemory(m_ui.pVertexBuffer);
                m_pDevice->destroyBuffer(m_ui.pVertexBuffer);
            }
            VK_CHECK_RESULT(m_pDevice->createBuffer(createInfo, &m_ui.pVertexBuffer));
            m_ui.vertexCount = imDrawData->TotalVtxCount;
            m_pDevice->mapMemory(m_ui.pVertexBuffer);
            updateCmdBuffers = true;
        }

        if(m_ui.pIndexBuffer == nullptr || (m_ui.indexCount = imDrawData->TotalVtxCount))
        {
            BufferCreateInfo createInfo = {.size   = static_cast<uint32_t>(vertexBufferSize),
                                           .usage  = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                           .domain = BufferDomain::Host};
            if(m_ui.pIndexBuffer)
            {
                m_pDevice->waitIdle();
                m_pDevice->unMapMemory(m_ui.pIndexBuffer);
                m_pDevice->destroyBuffer(m_ui.pIndexBuffer);
            }
            VK_CHECK_RESULT(m_pDevice->createBuffer(createInfo, &m_ui.pIndexBuffer));
            m_ui.indexCount = imDrawData->TotalIdxCount;
            m_pDevice->mapMemory(m_ui.pIndexBuffer);
            updateCmdBuffers = true;
        }

        // Upload data
        ImDrawVert* vtxDst = (ImDrawVert*)m_ui.pVertexBuffer->getMapped();
        ImDrawIdx*  idxDst = (ImDrawIdx*)m_ui.pIndexBuffer->getMapped();

        for(int n = 0; n < imDrawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }

        // Flush to make writes visible to GPU
        m_pDevice->flushMemory(m_ui.pVertexBuffer->getMemory());
        m_pDevice->flushMemory(m_ui.pIndexBuffer->getMemory());

        return updateCmdBuffers;
    }
    return true;
}
}  // namespace aph::vk
