#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "renderer/uiRenderer.h"
#include "vkl.hpp"

class scene_manager : public vkl::BaseApp {
public:
    scene_manager();

    void init() override;
    void run() override;
    void finish() override;

private:
    void setupWindow();
    void setupRenderer();

    void keyboardHandleDerive(int key, int scancode, int action, int mods);
    void mouseHandleDerive(double xposIn, double yposIn);

    void loadScene();
    void buildCommands();

private:
    std::shared_ptr<vkl::Light>  m_pointLight       = nullptr;
    std::shared_ptr<vkl::Light>  m_directionalLight = nullptr;
    std::shared_ptr<vkl::Entity> m_model            = nullptr;
    std::shared_ptr<vkl::Camera> m_defaultCamera    = nullptr;

    std::shared_ptr<vkl::Scene> m_scene;

    std::unique_ptr<vkl::VulkanSceneRenderer> m_sceneRenderer;
    std::unique_ptr<vkl::VulkanUIRenderer>    m_uiRenderer;
    std::unique_ptr<vkl::VulkanRenderer>      m_renderer;

    std::shared_ptr<vkl::Window> m_window;
    float                        m_deltaTime;
};

#endif // SCENE_MANAGER_H_
