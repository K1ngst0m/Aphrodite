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
class VulkanUIRenderer : IUIRenderer
{
public:
    VulkanUIRenderer(VulkanRenderer* pRenderer);
    ~VulkanUIRenderer();

    void init();
    bool update(float deltaTime);
    void draw(VulkanCommandBuffer* pCommandBuffer);
    void resize(uint32_t width, uint32_t height);
    void cleanup();

public:
    float getScaleFactor() const { return m_scale; }

public:
    void drawWithItemWidth(float itemWidth, std::function<void()>&& drawFunc) const;
    void drawWindow(std::string_view title, glm::vec2 pos, glm::vec2 size, std::function<void()>&& drawFunc) const;

public:
    bool header(const char* caption);
    bool checkBox(const char* caption, bool* value);
    bool checkBox(const char* caption, int32_t* value);
    bool radioButton(const char* caption, bool value);
    bool inputFloat(const char* caption, float* value, float step, uint32_t precision);
    bool sliderFloat(const char* caption, float* value, float min, float max);
    bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
    bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
    bool button(const char* caption);
    bool colorPicker(const char* caption, float* color);
    void text(const char* formatstr, ...);

private:
    struct PushConstBlock
    {
        glm::vec2 scale;
        glm::vec2 translate;
    } m_pushConstBlock;

    bool visible = true;
    bool updated = false;

    VulkanRenderer* m_pRenderer = {};
    VulkanDevice*   m_pDevice   = {};

    VulkanImage*     m_pFontImage  = {};
    VkSampler        m_fontSampler = {};
    VkDescriptorPool m_pool        = {};
    VkRenderPass     m_renderpass  = {};
    VulkanPipeline*  m_pPipeline   = {};

    VulkanBuffer* m_pVertexBuffer = {};
    VulkanBuffer* m_pIndexBuffer  = {};
    uint32_t      vertexCount     = {};
    uint32_t      indexCount      = {};

    VulkanDescriptorSetLayout* m_pSetLayout = {};
    VkDescriptorSet            m_set        = {};

    float m_scale = { 1.0f };
};
}  // namespace aph

#endif  // UIRENDERER_H_
