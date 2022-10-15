#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "renderer/api/vulkan/buffer.h"
#include "renderer/api/vulkan/commandBuffer.h"
#include "renderer/api/vulkan/image.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/uiRenderer.h"
#include "vulkanRenderer.h"

namespace vkl {
class VulkanUIRenderer : public UIRenderer {
public:
    static std::unique_ptr<VulkanUIRenderer> Create(VulkanRenderer *renderer, const std::shared_ptr<WindowData> &windowData);
    VulkanUIRenderer(VulkanRenderer *renderer, const std::shared_ptr<WindowData> &windowData);
    ~VulkanUIRenderer();

    void initUI();
    void initPipeline(VkPipelineCache pipelineCache, VulkanRenderPass *renderPass, VkFormat colorFormat, VkFormat depthFormat);
    void drawUI(VulkanCommandBuffer *command);
    void resize(uint32_t width, uint32_t height);
    void cleanup();
    bool update(float deltaTime);

private:
    VulkanRenderer *_renderer;
    VulkanDevice   *_device;
    DeletionQueue   _deletionQueue;

    VulkanBuffer *_vertexBuffer = nullptr;
    VulkanBuffer *_indexBuffer  = nullptr;

    uint32_t _vertexCount = 0;
    uint32_t _indexCount  = 0;

    VulkanPipeline *_pipeline;
    ShaderEffect   *_effect;

    VkDescriptorPool _descriptorPool;
    VkDescriptorSet  _descriptorSet;

    struct {
        VulkanImage     *image;
        VulkanImageView *view;
        VkSampler        sampler;
        float            scale = 1.0f;
    } _fontData;

    struct PushConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    } _pushConstBlock;
};
} // namespace vkl

#endif // UIRENDERER_H_
