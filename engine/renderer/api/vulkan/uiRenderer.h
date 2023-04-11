#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "api/vulkan/buffer.h"
#include "api/vulkan/commandBuffer.h"
#include "api/vulkan/image.h"
#include "api/vulkan/shader.h"
#include "renderer/uiRenderer.h"
#include "renderer.h"

namespace aph
{
// class VulkanUIRenderer : public UIRenderer {
// public:
//     static std::unique_ptr<VulkanUIRenderer> Create(const std::shared_ptr<VulkanRenderer>& renderer, const
//     std::shared_ptr<WindowData> &windowData); VulkanUIRenderer(const std::shared_ptr<VulkanRenderer>& renderer, const
//     std::shared_ptr<WindowData> &windowData); ~VulkanUIRenderer();

//     void initUI();
//     void initPipeline(VkPipelineCache pipelineCache, VulkanRenderPass *renderPass, VkFormat colorFormat, VkFormat
//     depthFormat); void drawUI(VulkanCommandBuffer *command); void resize(uint32_t width, uint32_t height); void
//     cleanup(); bool update(float deltaTime);

// private:
//     std::shared_ptr<VulkanRenderer> m_renderer = nullptr;

//     VulkanDevice   *m_device = nullptr;

//     VulkanBuffer *m_vertexBuffer = nullptr;
//     VulkanBuffer *m_indexBuffer  = nullptr;

//     uint32_t m_vertexCount = 0;
//     uint32_t m_indexCount  = 0;

//     VulkanPipeline *m_pipeline = nullptr;

//     VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
//     VkDescriptorSet  m_descriptorSet = VK_NULL_HANDLE;

//     struct {
//         VulkanImage     *image = nullptr;
//         VulkanImageView *view = nullptr;
//         VkSampler        sampler = VK_NULL_HANDLE;
//         float            scale = 1.0f;
//     } m_fontData;

//     struct PushConstBlock {
//         glm::vec2 scale;
//         glm::vec2 translate;
//     } m_pushConstBlock;
// };
}  // namespace aph

#endif  // UIRENDERER_H_
