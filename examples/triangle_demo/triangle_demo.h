#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vkl.hpp"
#include "vklRenderer.hpp"

class triangle_demo : public aph::BaseApp {
public:
    triangle_demo();

    void init() override;
    void run() override;
    void finish() override;

private:
    void setupPipeline();

    void buildCommands();

private:
    std::shared_ptr<aph::Window>         m_window;
    std::shared_ptr<aph::VulkanRenderer> m_renderer;
    aph::VulkanDevice                   *m_device = nullptr;
    float                                m_deltaTime;

    aph::VulkanPipeline   *m_demoPipeline;
};

#endif // SCENE_MANAGER_H_
