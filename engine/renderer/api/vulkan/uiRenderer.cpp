#include "uiRenderer.h"

#include "common/assetManager.h"

#include "api/vulkan/device.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <type_traits>

namespace aph {

// VulkanUIRenderer::VulkanUIRenderer(const std::shared_ptr<VulkanRenderer>& renderer, const std::shared_ptr<WindowData> &windowData)
//     : UIRenderer(windowData), m_renderer(renderer), m_device(renderer->getDevice()) {
//     // Init ImGui
//     ImGui::CreateContext();
//     // Color scheme
//     ImGuiStyle &style                       = ImGui::GetStyle();
//     style.Colors[ImGuiCol_TitleBg]          = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
//     style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
//     style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
//     style.Colors[ImGuiCol_MenuBarBg]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//     style.Colors[ImGuiCol_Header]           = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
//     style.Colors[ImGuiCol_HeaderActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//     style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//     style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
//     style.Colors[ImGuiCol_CheckMark]        = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
//     style.Colors[ImGuiCol_SliderGrab]       = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//     style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
//     style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
//     style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
//     style.Colors[ImGuiCol_Button]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//     style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
//     style.Colors[ImGuiCol_ButtonActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
//     // Dimensions
//     ImGuiIO &io        = ImGui::GetIO();
//     io.FontGlobalScale = m_fontData.scale;
// }

// VulkanUIRenderer::~VulkanUIRenderer() {
//     if (ImGui::GetCurrentContext()) {
//         ImGui::DestroyContext();
//     }
// }

// void VulkanUIRenderer::initUI() {
//     ImGuiIO &io = ImGui::GetIO();

//     // Create font texture
//     unsigned char    *fontData;
//     int               texWidth, texHeight;
//     const std::string filename = AssetManager::GetFontDir() / "Roboto-Medium.ttf";
//     io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * m_fontData.scale);
//     io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
//     VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

//     // SRS - Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
//     ImGuiStyle &style = ImGui::GetStyle();
//     style.ScaleAllSizes(m_fontData.scale);

//     // Create target image for copy
//     {
//         ImageCreateInfo imageInfo{};
//         imageInfo.imageType     = IMAGE_TYPE_2D;
//         imageInfo.format        = FORMAT_R8G8B8A8_UNORM;
//         imageInfo.extent.width  = texWidth;
//         imageInfo.extent.height = texHeight;
//         imageInfo.extent.depth  = 1;
//         imageInfo.mipLevels     = 1;
//         imageInfo.arrayLayers   = 1;
//         imageInfo.samples       = SAMPLE_COUNT_1_BIT;
//         imageInfo.tiling        = IMAGE_TILING_OPTIMAL;
//         imageInfo.usage         = IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_DST_BIT;
//         imageInfo.property      = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//         VK_CHECK_RESULT(m_device->createImage(imageInfo, &m_fontData.image));
//     }

//     // Image view
//     {
//         ImageViewCreateInfo viewInfo{};
//         viewInfo.viewType                    = IMAGE_VIEW_TYPE_2D;
//         viewInfo.format                      = FORMAT_R8G8B8A8_UNORM;
//         viewInfo.subresourceRange.levelCount = 1;
//         viewInfo.subresourceRange.layerCount = 1;
//         VK_CHECK_RESULT(m_device->createImageView(viewInfo, &m_fontData.view, m_fontData.image));
//     }

//     // font data upload
//     {
//         VulkanBuffer *stagingBuffer = nullptr;
//         BufferCreateInfo createInfo{};
//         createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
//         createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
//         createInfo.size     = uploadSize;
//         m_device->createBuffer(createInfo, &stagingBuffer);
//         stagingBuffer->map();
//         stagingBuffer->copyTo(fontData, uploadSize);
//         stagingBuffer->unmap();

//         // Copy buffer data to font image
//         m_device->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *copyCmd){
//             copyCmd->cmdTransitionImageLayout(m_fontData.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
//             copyCmd->cmdCopyBufferToImage(stagingBuffer, m_fontData.image);
//             copyCmd->cmdTransitionImageLayout(m_fontData.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//         });
//         m_device->destroyBuffer(stagingBuffer);
//     }

//     // Font texture Sampler
//     {
//         VkSamplerCreateInfo samplerInfo = aph::init::samplerCreateInfo();
//         samplerInfo.magFilter           = VK_FILTER_LINEAR;
//         samplerInfo.minFilter           = VK_FILTER_LINEAR;
//         samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//         samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//         samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//         samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//         samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
//         VK_CHECK_RESULT(vkCreateSampler(m_device->getHandle(), &samplerInfo, nullptr, &m_fontData.sampler));
//     }

// }

// void VulkanUIRenderer::resize(uint32_t width, uint32_t height) {
//     ImGuiIO &io    = ImGui::GetIO();
//     io.DisplaySize = ImVec2((float)(width), (float)(height));
// }

// void VulkanUIRenderer::cleanup() {
//     m_device->destroyBuffer(m_vertexBuffer);
//     m_device->destroyBuffer(m_indexBuffer);
//     m_device->destroyImageView(m_fontData.view);
//     m_device->destroyImage(m_fontData.image);
//     vkDestroySampler(m_device->getHandle(), m_fontData.sampler, nullptr);

//     m_device->destroyPipeline(m_pipeline);

//     vkDestroyDescriptorPool(m_device->getHandle(), m_descriptorPool, nullptr);
// }

// bool VulkanUIRenderer::update(float deltaTime) {
//     ImDrawData *imDrawData       = ImGui::GetDrawData();
//     bool        updateCmdBuffers = false;

//     if (!imDrawData) {
//         return false;
//     };

//     // Note: Alignment is done inside buffer creation
//     uint32_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
//     uint32_t indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

//     // Update buffers only if vertex or index count has been changed compared to current buffer size
//     if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
//         return false;
//     }

//     // Vertex buffer
//     if (m_vertexBuffer == nullptr){
//         BufferCreateInfo createInfo{
//             .size     = vertexBufferSize,
//             .usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT,
//             .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//         };
//         VK_CHECK_RESULT(m_device->createBuffer(createInfo, &m_vertexBuffer));
//     }
//     if ((m_vertexBuffer->getHandle() == VK_NULL_HANDLE) || (m_vertexCount != imDrawData->TotalVtxCount)) {
//         m_vertexBuffer->unmap();
//         m_device->destroyBuffer(m_vertexBuffer);
//         BufferCreateInfo createInfo{
//             .size     = vertexBufferSize,
//             .usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT,
//             .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//         };
//         VK_CHECK_RESULT(m_device->createBuffer(createInfo, &m_vertexBuffer));
//         m_vertexCount = imDrawData->TotalVtxCount;
//         m_vertexBuffer->unmap();
//         m_vertexBuffer->map();
//         updateCmdBuffers = true;
//     }

//     // Index buffer
//     if (m_indexBuffer == nullptr){
//         BufferCreateInfo createInfo{
//             .size     = indexBufferSize,
//             .usage    = BUFFER_USAGE_INDEX_BUFFER_BIT,
//             .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//         };
//         VK_CHECK_RESULT(m_device->createBuffer(createInfo, &m_indexBuffer));
//     }
//     if ((m_indexBuffer->getHandle() == VK_NULL_HANDLE) || (m_indexCount < imDrawData->TotalIdxCount)) {
//         m_indexBuffer->unmap();
//         m_device->destroyBuffer(m_indexBuffer);
//         BufferCreateInfo createInfo{
//             .size     = indexBufferSize,
//             .usage    = BUFFER_USAGE_INDEX_BUFFER_BIT,
//             .property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//         };
//         VK_CHECK_RESULT(m_device->createBuffer(createInfo, &m_indexBuffer));
//         m_indexCount = imDrawData->TotalIdxCount;
//         m_indexBuffer->map();
//         updateCmdBuffers = true;
//     }

//     // Upload data
//     ImDrawVert *vtxDst = (ImDrawVert *)m_vertexBuffer->getMapped();
//     ImDrawIdx  *idxDst = (ImDrawIdx *)m_indexBuffer->getMapped();

//     for (int n = 0; n < imDrawData->CmdListsCount; n++) {
//         const ImDrawList *cmd_list = imDrawData->CmdLists[n];
//         memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
//         memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
//         vtxDst += cmd_list->VtxBuffer.Size;
//         idxDst += cmd_list->IdxBuffer.Size;
//     }

//     // Flush to make writes visible to GPU
//     m_vertexBuffer->flush();
//     m_indexBuffer->flush();

//     return updateCmdBuffers;
// }

// void VulkanUIRenderer::initPipeline(VkPipelineCache pipelineCache, VulkanRenderPass *renderPass, VkFormat colorFormat, VkFormat depthFormat) {
//     GraphicsPipelineCreateInfo pipelineCI{};
//     // Setup graphics pipeline for UI rendering
//     pipelineCI.inputAssembly = aph::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
//     pipelineCI.rasterizer    = aph::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

//     // Enable blending
//     VkPipelineColorBlendAttachmentState blendAttachmentState{};
//     blendAttachmentState.blendEnable         = VK_TRUE;
//     blendAttachmentState.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//     blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//     blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//     blendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
//     blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//     blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//     blendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;

//     pipelineCI.colorBlendAttachment = blendAttachmentState;

//     pipelineCI.depthStencil  = aph::init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
//     pipelineCI.multisampling = aph::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

//     VkPipelineViewportStateCreateInfo viewportState =
//         aph::init::pipelineViewportStateCreateInfo(1, 1, 0);

//     std::vector<VkDynamicState> dynamicStateEnables = {
//         VK_DYNAMIC_STATE_VIEWPORT,
//         VK_DYNAMIC_STATE_SCISSOR};
//     pipelineCI.dynamicState = aph::init::pipelineDynamicStateCreateInfo(dynamicStateEnables);

//     // Vertex bindings an attributes based on ImGui vertex definition
//     std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
//         aph::init::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
//     };
//     std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
//         aph::init::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)), // Location 0: Position
//         aph::init::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)), // Location 1: UV
//         aph::init::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)), // Location 0: Color
//     };

//     VkPipelineVertexInputStateCreateInfo &vertexInputState = pipelineCI.vertexInputInfo;
//     vertexInputState                                       = aph::init::pipelineVertexInputStateCreateInfo();
//     vertexInputState.vertexBindingDescriptionCount         = static_cast<uint32_t>(vertexInputBindings.size());
//     vertexInputState.pVertexBindingDescriptions            = vertexInputBindings.data();
//     vertexInputState.vertexAttributeDescriptionCount       = static_cast<uint32_t>(vertexInputAttributes.size());
//     vertexInputState.pVertexAttributeDescriptions          = vertexInputAttributes.data();


//     // build effect
//     {
//         VulkanDescriptorSetLayout * pLayout = nullptr;
//         std::vector<VkDescriptorSetLayoutBinding> bindings {
//             aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
//         };
//         VkDescriptorSetLayoutCreateInfo layoutCreateInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
//         VK_CHECK_RESULT(m_device->createDescriptorSetLayout(&layoutCreateInfo, &pLayout));

//         auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "ui";
//         pipelineCI.setLayouts.push_back(pLayout);
//         pipelineCI.constants.push_back(aph::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0));
//         pipelineCI.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] = m_renderer->getShaderCache()->getShaders(shaderDir / "uioverlay.vert.spv");
//         pipelineCI.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = m_renderer->getShaderCache()->getShaders(shaderDir / "uioverlay.frag.spv");

//         VK_CHECK_RESULT(m_device->createGraphicsPipeline(pipelineCI, renderPass, &m_pipeline));
//     }

//     // Descriptor pool
//     {
//         std::vector<VkDescriptorPoolSize> poolSizes = {
//             aph::init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

//         VkDescriptorPoolCreateInfo descriptorPoolInfo = aph::init::descriptorPoolCreateInfo(poolSizes, 2);
//         VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->getHandle(), &descriptorPoolInfo, nullptr, &m_descriptorPool));

//         // Descriptor set
//         VkDescriptorSetAllocateInfo allocInfo = aph::init::descriptorSetAllocateInfo(m_descriptorPool, &m_pipeline->getDescriptorSetLayout(0)->getHandle(), 1);
//         VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device->getHandle(), &allocInfo, &m_descriptorSet));
//         VkDescriptorImageInfo fontDescriptor = aph::init::descriptorImageInfo(
//             m_fontData.sampler,
//             m_fontData.view->getHandle(),
//             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//         std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
//             aph::init::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)};
//         vkUpdateDescriptorSets(m_device->getHandle(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
//     }
// }

// void VulkanUIRenderer::drawUI(VulkanCommandBuffer *command) {
//     static bool updated = false;

//     if (!updated){
//         ImGuiIO& io = ImGui::GetIO();

//         io.DisplaySize = ImVec2((float)m_renderer->getWindowWidth(), (float)m_renderer->getWindowHeight());

//         ImGui::NewFrame();

//         ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
//         ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
//         ImGui::ShowDemoWindow();
//         ImGui::End();
//         ImGui::PopStyleVar();
//         ImGui::Render();

//         // TODO
//         updated = update(0.0f);
//     }

//     ImDrawData *imDrawData   = ImGui::GetDrawData();
//     int32_t     vertexOffset = 0;
//     int32_t     indexOffset  = 0;

//     if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
//         return;
//     }

//     ImGuiIO &io = ImGui::GetIO();

//     command->cmdBindPipeline(m_pipeline);
//     command->cmdBindDescriptorSet(m_pipeline, 0, 1, &m_descriptorSet);
//     m_pushConstBlock.scale     = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
//     m_pushConstBlock.translate = glm::vec2(-1.0f);
//     command->cmdPushConstants(m_pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_pushConstBlock), &m_pushConstBlock);

//     VkDeviceSize offsets[1] = {0};
//     command->cmdBindVertexBuffers(0, 1, m_vertexBuffer, offsets);
//     command->cmdBindIndexBuffers(m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

//     for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
//         const ImDrawList *cmd_list = imDrawData->CmdLists[i];
//         for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
//             const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
//             VkRect2D         scissorRect;
//             scissorRect.offset.x      = std::max((int32_t)(pcmd->ClipRect.x), 0);
//             scissorRect.offset.y      = std::max((int32_t)(pcmd->ClipRect.y), 0);
//             scissorRect.extent.width  = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
//             scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
//             command->cmdSetSissor(&scissorRect);
//             command->cmdDrawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
//             indexOffset += pcmd->ElemCount;
//         }
//         vertexOffset += cmd_list->VtxBuffer.Size;
//     }
// }
// std::unique_ptr<VulkanUIRenderer> VulkanUIRenderer::Create(const std::shared_ptr<VulkanRenderer>& renderer, const std::shared_ptr<WindowData> &windowData) {
//     auto instance = std::make_unique<VulkanUIRenderer>(renderer, windowData);
//     return instance;
// }
} // namespace aph
