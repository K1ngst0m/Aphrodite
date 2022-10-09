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
    std::shared_ptr<vkl::Window>   m_window;
    std::unique_ptr<vkl::Renderer> m_renderer;
    float                          m_deltaTime;
};

#endif // SCENE_MANAGER_H_
