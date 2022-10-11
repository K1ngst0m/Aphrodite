#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

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

    std::shared_ptr<vkl::SceneManager>  m_scene;
    std::shared_ptr<vkl::SceneRenderer> m_sceneRenderer;

    std::shared_ptr<vkl::Window>   m_window;
    std::unique_ptr<vkl::Renderer> m_renderer;
    float                          m_deltaTime;
};

#endif // SCENE_MANAGER_H_
