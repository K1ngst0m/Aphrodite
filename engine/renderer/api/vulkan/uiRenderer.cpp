#include "uiRenderer.h"

#include "common/assetManager.h"

#include "api/vulkan/device.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <type_traits>

namespace aph
{
VulkanUIRenderer::VulkanUIRenderer(VulkanRenderer* renderer) :
    IUIRenderer(renderer->getWindow()),
    m_pRenderer(renderer),
    m_pDevice(renderer->getDevice())
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Setup Dear ImGui style
    io.FontGlobalScale = m_scale;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // TODO
    init();
}

VulkanUIRenderer::~VulkanUIRenderer()
{
    if(ImGui::GetCurrentContext()) { ImGui::DestroyContext(); }
}

void VulkanUIRenderer::init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Create font texture
    {
        unsigned char*    fontData;
        int               texWidth, texHeight;
        const std::string filename = AssetManager::GetFontDir() / "Roboto-Medium.ttf";
        io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * m_scale);
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        VkDeviceSize uploadSize = texWidth * texHeight * 4;

        // SRS - Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(m_scale);

        std::vector<uint8_t> imageData(fontData, fontData + uploadSize);
        ImageCreateInfo      creatInfo{
                 .extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
                 .usage  = IMAGE_USAGE_SAMPLED_BIT,
                 .format = Format::R8G8B8A8_UNORM,
                 .tiling = ImageTiling::OPTIMAL,
        };
        m_pDevice->createDeviceLocalImage(creatInfo, &m_pFontImage, imageData);
        m_pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer* pCmd) {
            pCmd->transitionImageLayout(m_pFontImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
    }

    // font sampler
    {
        VkSamplerCreateInfo samplerInfo = aph::init::samplerCreateInfo();
        samplerInfo.magFilter           = VK_FILTER_LINEAR;
        samplerInfo.minFilter           = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(m_pDevice->createSampler(samplerInfo, &m_fontSampler));
    }

    // setup descriptor
    {
        std::vector<ResourcesBinding> bindings{
            {ResourceType::COMBINE_SAMPLER_IMAGE, {ShaderStage::FS}},
        };
        m_pDevice->createDescriptorSetLayout(bindings, &m_pSetLayout);

        VkDescriptorImageInfo      fontDescriptor = {m_fontSampler, m_pFontImage->getImageView()->getHandle(),
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        std::vector<ResourceWrite> writes{
            {&fontDescriptor, {}},
        };

        m_set = m_pSetLayout->allocateSet(writes);
    }

    // setup pipeline
    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        auto                       shaderDir    = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "ui";
        std::vector<VkFormat>      colorFormats = {m_pRenderer->getSwapChain()->getSurfaceFormat()};
        pipelineCreateInfo.renderingCreateInfo  = VkPipelineRenderingCreateInfo{
             .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
             .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
             .pColorAttachmentFormats = colorFormats.data(),
             .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.depthStencil =
            aph::init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

        pipelineCreateInfo.setLayouts = {m_pSetLayout};
        pipelineCreateInfo.constants  = {{utils::VkCast(ShaderStage::VS), 0, sizeof(PushConstBlock)}};
        pipelineCreateInfo.shaderMapList[ShaderStage::VS] = m_pRenderer->getShaders(shaderDir / "uioverlay.vert.spv");
        pipelineCreateInfo.shaderMapList[ShaderStage::FS] = m_pRenderer->getShaders(shaderDir / "uioverlay.frag.spv");

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
        pipelineCreateInfo.colorBlendAttachment = blendAttachmentState;
        pipelineCreateInfo.multisampling        = aph::init::pipelineMultisampleStateCreateInfo(
            static_cast<VkSampleCountFlagBits>(m_pRenderer->getConfig().sampleCount));

        // Vertex bindings an attributes based on ImGui vertex definition
        std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
            {0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX},
        };
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
            {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},   // Location 0: Position
            {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},    // Location 1: UV
            {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)},  // Location 0: Color
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = aph::init::pipelineVertexInputStateCreateInfo();
        vertexInputInfo.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertexInputBindings.size());
        vertexInputInfo.pVertexBindingDescriptions           = vertexInputBindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions         = vertexInputAttributes.data();
        pipelineCreateInfo.vertexInputInfo                   = vertexInputInfo;

        VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_pPipeline));
    }
}

bool VulkanUIRenderer::update(float deltaTime)
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
    if(m_pVertexBuffer == nullptr || (m_vertexCount != imDrawData->TotalVtxCount))
    {
        BufferCreateInfo createInfo = {.size     = static_cast<uint32_t>(vertexBufferSize),
                                       .usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                       .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT};
        if(m_pVertexBuffer)
        {
            m_pDevice->waitIdle();
            m_pDevice->unMapMemory(m_pVertexBuffer);
            m_pDevice->destroyBuffer(m_pVertexBuffer);
        }
        VK_CHECK_RESULT(m_pDevice->createBuffer(createInfo, &m_pVertexBuffer));
        m_vertexCount = imDrawData->TotalVtxCount;
        m_pDevice->mapMemory(m_pVertexBuffer);
        updateCmdBuffers = true;
    }

    if(m_pIndexBuffer == nullptr || (m_indexCount != imDrawData->TotalVtxCount))
    {
        BufferCreateInfo createInfo = {.size     = static_cast<uint32_t>(vertexBufferSize),
                                       .usage    = BUFFER_USAGE_INDEX_BUFFER_BIT,
                                       .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT};
        if(m_pIndexBuffer)
        {
            m_pDevice->waitIdle();
            m_pDevice->unMapMemory(m_pIndexBuffer);
            m_pDevice->destroyBuffer(m_pIndexBuffer);
        }
        VK_CHECK_RESULT(m_pDevice->createBuffer(createInfo, &m_pIndexBuffer));
        m_indexCount = imDrawData->TotalIdxCount;
        m_pDevice->mapMemory(m_pIndexBuffer);
        updateCmdBuffers = true;
    }

    // Upload data
    ImDrawVert* vtxDst = (ImDrawVert*)m_pVertexBuffer->getMapped();
    ImDrawIdx*  idxDst = (ImDrawIdx*)m_pIndexBuffer->getMapped();

    for(int n = 0; n < imDrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }

    // Flush to make writes visible to GPU
    m_pDevice->flushMemory(m_pVertexBuffer->getMemory());
    m_pDevice->flushMemory(m_pIndexBuffer->getMemory());

    return updateCmdBuffers;
}

void VulkanUIRenderer::draw(VulkanCommandBuffer* pCommandBuffer)
{
    ImDrawData* imDrawData   = ImGui::GetDrawData();
    int32_t     vertexOffset = 0;
    int32_t     indexOffset  = 0;

    if((!imDrawData) || (imDrawData->CmdListsCount == 0)) { return; }

    ImGuiIO& io = ImGui::GetIO();

    pCommandBuffer->bindPipeline(m_pPipeline);
    pCommandBuffer->bindDescriptorSet(m_pPipeline, 0, 1, &m_set);

    m_pushConstBlock.scale     = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    m_pushConstBlock.translate = glm::vec2(-1.0f);
    pCommandBuffer->pushConstants(m_pPipeline, {ShaderStage::VS}, 0, sizeof(PushConstBlock), &m_pushConstBlock);

    pCommandBuffer->bindVertexBuffers(0, 1, m_pVertexBuffer, {0});
    pCommandBuffer->bindIndexBuffers(m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

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

void VulkanUIRenderer::resize(uint32_t width, uint32_t height)
{
    ImGuiIO& io    = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(width), (float)(height));
}

void VulkanUIRenderer::cleanup()
{
    m_pDevice->destroyBuffer(m_pVertexBuffer);
    m_pDevice->destroyBuffer(m_pIndexBuffer);

    m_pDevice->destroyImage(m_pFontImage);

    m_pDevice->destroySampler(m_fontSampler);
    m_pDevice->destroyDescriptorSetLayout(m_pSetLayout);
    m_pDevice->destroyPipeline(m_pPipeline);
}

void VulkanUIRenderer::text(const char* formatstr, ...)
{
    va_list args;
    va_start(args, formatstr);
    ImGui::TextV(formatstr, args);
    va_end(args);
}

bool VulkanUIRenderer::colorPicker(const char* caption, float* color)
{
    bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::button(const char* caption)
{
    bool res = ImGui::Button(caption);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
{
    if(items.empty()) { return false; }
    std::vector<const char*> charitems;
    charitems.reserve(items.size());
    for(auto& item : items)
    {
        charitems.push_back(item.c_str());
    }
    uint32_t itemCount = static_cast<uint32_t>(charitems.size());
    bool     res       = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
{
    bool res = ImGui::SliderInt(caption, value, min, max);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::inputFloat(const char* caption, float* value, float step, uint32_t precision)
{
    return false;
    // bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
    // if(res)
    // {
    //     updated = true;
    // };
    // return res;
}
bool VulkanUIRenderer::sliderFloat(const char* caption, float* value, float min, float max)
{
    bool res = ImGui::SliderFloat(caption, value, min, max);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::checkBox(const char* caption, int32_t* value)
{
    bool val = (*value == 1);
    bool res = ImGui::Checkbox(caption, &val);
    *value   = val;
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::checkBox(const char* caption, bool* value)
{
    bool res = ImGui::Checkbox(caption, value);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::radioButton(const char* caption, bool value)
{
    bool res = ImGui::RadioButton(caption, value);
    if(res) { updated = true; };
    return res;
}
bool VulkanUIRenderer::header(const char* caption)
{
    return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

void VulkanUIRenderer::drawWithItemWidth(float itemWidth, std::function<void()>&& drawFunc) const
{
    ImGui::PushItemWidth(itemWidth * m_scale);
    drawFunc();
    ImGui::PopItemWidth();
}

void VulkanUIRenderer::drawWindow(std::string_view title, glm::vec2 pos, glm::vec2 size,
                                  std::function<void()>&& drawFunc) const
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(pos.x * m_scale, pos.y * m_scale));
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
    ImGui::Begin(title.data(), nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    drawFunc();
    ImGui::End();
    ImGui::PopStyleVar();
}
}  // namespace aph
