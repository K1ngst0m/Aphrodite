#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vkl.hpp"
#include "vklRenderer.hpp"

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

    void setupScene();

private:
    std::shared_ptr<vkl::SceneNode> m_modelNode            = nullptr;
    std::shared_ptr<vkl::SceneNode> m_pointLightNode       = nullptr;
    std::shared_ptr<vkl::SceneNode> m_directionalLightNode = nullptr;
    std::shared_ptr<vkl::SceneNode> m_cameraNode           = nullptr;

    std::shared_ptr<vkl::Scene> m_scene;

    std::unique_ptr<vkl::VulkanSceneRenderer> m_sceneRenderer;
    // std::unique_ptr<vkl::VulkanUIRenderer>    m_uiRenderer;
    std::shared_ptr<vkl::VulkanRenderer>      m_renderer;

    std::shared_ptr<vkl::Window> m_window;
    float                        m_deltaTime;
};

#endif // SCENE_MANAGER_H_
