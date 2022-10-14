#include "uiRenderer.h"
#include "commandBuffer.h"
#include "common/assetManager.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "renderer/api/vulkan/physicalDevice.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/api/vulkan/vkUtils.h"
#include "renderer/gpuResource.h"
#include "renderpass.h"
#include "shader.h"
#include "spirv_reflect/include/spirv/unified1/spirv.h"
#include "vulkan/vulkan_core.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <type_traits>

namespace vkl {
VulkanUIRenderer::VulkanUIRenderer(VulkanRenderer *renderer, const std::shared_ptr<WindowData> &windowData)
    : UIRenderer(windowData), _renderer(renderer), _device(renderer->getDevice()) {
    // Init ImGui
    ImGui::CreateContext();
    // Color scheme
    ImGuiStyle &style                       = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg]          = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header]           = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark]        = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    // Dimensions
    ImGuiIO &io        = ImGui::GetIO();
    io.FontGlobalScale = _fontData.scale;
}

VulkanUIRenderer::~VulkanUIRenderer() {
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
}

void VulkanUIRenderer::initUI() {
    ImGuiIO &io = ImGui::GetIO();

    // Create font texture
    unsigned char    *fontData;
    int               texWidth, texHeight;
    const std::string filename = AssetManager::GetFontDir() / "Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * _fontData.scale);
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

    // SRS - Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(_fontData.scale);

    // Create target image for copy
    {
        ImageCreateInfo imageInfo{};
        imageInfo.imageType     = IMAGE_TYPE_2D;
        imageInfo.format        = FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width  = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.samples       = SAMPLE_COUNT_1_BIT;
        imageInfo.tiling        = IMAGE_TILING_OPTIMAL;
        imageInfo.usage         = IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.property      = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VK_CHECK_RESULT(_device->createImage(&imageInfo, &_fontData.image));
    }

    // Image view
    {
        ImageViewCreateInfo viewInfo{};
        viewInfo.viewType                    = IMAGE_VIEW_TYPE_2D;
        viewInfo.format                      = FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK_RESULT(_device->createImageView(&viewInfo, &_fontData.view, _fontData.image));
    }

    // font data upload
    {
        VulkanBuffer *stagingBuffer = nullptr;
        BufferCreateInfo createInfo{};
        createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
        createInfo.size     = uploadSize;
        _device->createBuffer(&createInfo, &stagingBuffer);
        stagingBuffer->map();
        stagingBuffer->copyTo(fontData, uploadSize);
        stagingBuffer->unmap();

        // Copy buffer data to font image
        auto *copyCmd = _device->beginSingleTimeCommands(QUEUE_TYPE_TRANSFER);
        copyCmd->cmdTransitionImageLayout(_fontData.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyCmd->cmdCopyBufferToImage(stagingBuffer, _fontData.image);
        copyCmd->cmdTransitionImageLayout(_fontData.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        _device->endSingleTimeCommands(copyCmd, QUEUE_TYPE_TRANSFER);
        _device->destroyBuffer(stagingBuffer);
    }

    // Font texture Sampler
    {
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.magFilter           = VK_FILTER_LINEAR;
        samplerInfo.minFilter           = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(vkCreateSampler(_device->getHandle(), &samplerInfo, nullptr, &_fontData.sampler));
    }

    // build effect
    {
        auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "ui";
        EffectBuilder effectBuilder(_device);
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkPushConstantRange pushConstantRange = vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
        _effect = effectBuilder.pushSetLayout(setLayoutBindings)
                      .pushConstantRanges(pushConstantRange)
                      .pushShaderStages(_renderer->getShaderCache().getShaders(_device, shaderDir / "uioverlay.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
                      .pushShaderStages(_renderer->getShaderCache().getShaders(_device, shaderDir / "uioverlay.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT)
                      .build();
    }

    // Descriptor pool
    {
        std::vector<VkDescriptorPoolSize> poolSizes = {
            vkl::init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

        VkDescriptorPoolCreateInfo descriptorPoolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, 2);
        VK_CHECK_RESULT(vkCreateDescriptorPool(_device->getHandle(), &descriptorPoolInfo, nullptr, &_descriptorPool));

        // Descriptor set
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, _effect->getDescriptorSetLayout(0), 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->getHandle(), &allocInfo, &_descriptorSet));
        VkDescriptorImageInfo fontDescriptor = vkl::init::descriptorImageInfo(
            _fontData.sampler,
            _fontData.view->getHandle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            vkl::init::writeDescriptorSet(_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)};
        vkUpdateDescriptorSets(_device->getHandle(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

}

void VulkanUIRenderer::resize(uint32_t width, uint32_t height) {
    ImGuiIO &io    = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(width), (float)(height));
}

void VulkanUIRenderer::cleanup() {
    _device->destroyBuffer(_vertexBuffer);
    _device->destroyBuffer(_indexBuffer);
    _device->destroyImageView(_fontData.view);
    _device->destroyImage(_fontData.image);
    vkDestroySampler(_device->getHandle(), _fontData.sampler, nullptr);

    _effect->destroy();
    _device->destroyPipeline(_pipeline);

    vkDestroyDescriptorPool(_device->getHandle(), _descriptorPool, nullptr);
}

bool VulkanUIRenderer::update() {
    ImDrawData *imDrawData       = ImGui::GetDrawData();
    bool        updateCmdBuffers = false;

    if (!imDrawData) {
        return false;
    };

    // Note: Alignment is done inside buffer creation
    uint32_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    uint32_t indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    // Update buffers only if vertex or index count has been changed compared to current buffer size
    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
        return false;
    }

    // Vertex buffer
    if (_vertexBuffer == nullptr){
        BufferCreateInfo createInfo{
            .size     = vertexBufferSize,
            .usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(_device->createBuffer(&createInfo, &_vertexBuffer));
    }
    if ((_vertexBuffer->getHandle() == VK_NULL_HANDLE) || (_vertexCount != imDrawData->TotalVtxCount)) {
        _vertexBuffer->unmap();
        _device->destroyBuffer(_vertexBuffer);
        BufferCreateInfo createInfo{
            .size     = vertexBufferSize,
            .usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(_device->createBuffer(&createInfo, &_vertexBuffer));
        _vertexCount = imDrawData->TotalVtxCount;
        _vertexBuffer->unmap();
        _vertexBuffer->map();
        updateCmdBuffers = true;
    }

    // Index buffer
    if (_indexBuffer == nullptr){
        BufferCreateInfo createInfo{
            .size     = indexBufferSize,
            .usage    = BUFFER_USAGE_INDEX_BUFFER_BIT,
            .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(_device->createBuffer(&createInfo, &_indexBuffer));
    }
    if ((_indexBuffer->getHandle() == VK_NULL_HANDLE) || (_indexCount < imDrawData->TotalIdxCount)) {
        _indexBuffer->unmap();
        _device->destroyBuffer(_indexBuffer);
        BufferCreateInfo createInfo{
            .size     = indexBufferSize,
            .usage    = BUFFER_USAGE_INDEX_BUFFER_BIT,
            .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(_device->createBuffer(&createInfo, &_indexBuffer));
        _indexCount = imDrawData->TotalIdxCount;
        _indexBuffer->map();
        updateCmdBuffers = true;
    }

    // Upload data
    ImDrawVert *vtxDst = (ImDrawVert *)_vertexBuffer->getMapped();
    ImDrawIdx  *idxDst = (ImDrawIdx *)_indexBuffer->getMapped();

    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList *cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }

    // Flush to make writes visible to GPU
    _vertexBuffer->flush();
    _indexBuffer->flush();

    return updateCmdBuffers;
}

void VulkanUIRenderer::initPipeline(VkPipelineCache pipelineCache, VulkanRenderPass *renderPass, const VkFormat colorFormat, const VkFormat depthFormat) {
    PipelineCreateInfo createInfo{};
    // Setup graphics pipeline for UI rendering
    createInfo._inputAssembly = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    createInfo._rasterizer    = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    // Enable blending
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable         = VK_TRUE;
    blendAttachmentState.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;

    createInfo._colorBlendAttachment = blendAttachmentState;

    createInfo._depthStencil  = vkl::init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
    createInfo._multisampling = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    VkPipelineViewportStateCreateInfo viewportState =
        vkl::init::pipelineViewportStateCreateInfo(1, 1, 0);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    createInfo._dynamicState = vkl::init::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    // Vertex bindings an attributes based on ImGui vertex definition
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        vkl::init::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
    };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        vkl::init::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)), // Location 0: Position
        vkl::init::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)), // Location 1: UV
        vkl::init::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)), // Location 0: Color
    };

    VkPipelineVertexInputStateCreateInfo &vertexInputState = createInfo._vertexInputInfo;
    vertexInputState                                       = vkl::init::pipelineVertexInputStateCreateInfo();
    vertexInputState.vertexBindingDescriptionCount         = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions            = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount       = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions          = vertexInputAttributes.data();

    VK_CHECK_RESULT(_device->createGraphicsPipeline(&createInfo, _effect.get(), renderPass, &_pipeline));
}

void VulkanUIRenderer::drawUI(VulkanCommandBuffer *command) {
    static bool updated = false;

    if (!updated){
        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2((float)_renderer->getWindowWidth(), (float)_renderer->getWindowHeight());

        ImGui::NewFrame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::ShowDemoWindow();
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::Render();

        updated = update();
    }

    ImDrawData *imDrawData   = ImGui::GetDrawData();
    int32_t     vertexOffset = 0;
    int32_t     indexOffset  = 0;

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
        return;
    }

    ImGuiIO &io = ImGui::GetIO();

    command->cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    command->cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1, &_descriptorSet);
    _pushConstBlock.scale     = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    _pushConstBlock.translate = glm::vec2(-1.0f);
    command->cmdPushConstants(_pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(_pushConstBlock), &_pushConstBlock);

    VkDeviceSize offsets[1] = {0};
    command->cmdBindVertexBuffers(0, 1, _vertexBuffer, offsets);
    command->cmdBindIndexBuffers(_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
        const ImDrawList *cmd_list = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
            VkRect2D         scissorRect;
            scissorRect.offset.x      = std::max((int32_t)(pcmd->ClipRect.x), 0);
            scissorRect.offset.y      = std::max((int32_t)(pcmd->ClipRect.y), 0);
            scissorRect.extent.width  = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
            command->cmdSetSissor(&scissorRect);
            command->cmdDrawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
            indexOffset += pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size;
    }
}
} // namespace vkl
