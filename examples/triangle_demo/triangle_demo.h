#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class triangle_demo : public aph::BaseApp
{
public:
    triangle_demo();

    void init() override;
    void run() override;
    void finish() override;

private:
    void setupPipeline();

    void buildCommands();

private:
    std::shared_ptr<aph::Window> m_window;
    std::shared_ptr<aph::VulkanRenderer> m_renderer;

    aph::VulkanRenderPass *m_pRenderPass = nullptr;
    std::vector<aph::VulkanFramebuffer *> m_framebuffers;
    std::vector<aph::VulkanImage *> m_colorAttachments;
    std::vector<aph::VulkanImage *> m_depthAttachments;

    aph::VulkanPipeline *m_demoPipeline;
    aph::VulkanDevice *m_device = nullptr;

    float m_deltaTime;
};

#endif  // SCENE_MANAGER_H_
