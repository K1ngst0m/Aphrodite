#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vkl.hpp"

class triangle_demo : public vkl::BaseApp {
public:
    triangle_demo();

    void init() override;
    void run() override;
    void finish() override;

private:
    void setupPipeline();

    void buildCommands();

private:
    std::shared_ptr<vkl::Window>         m_window;
    std::unique_ptr<vkl::VulkanRenderer> m_renderer;
    vkl::VulkanDevice                   *m_device = nullptr;
    float                                m_deltaTime;

    vkl::VulkanPipeline   *m_demoPipeline;
};

#endif // SCENE_MANAGER_H_
