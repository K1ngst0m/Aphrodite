#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#define VKL_SESSION_USING_SCENE_MANAGER

#include "app.h"

class scene_manager : public vkl::vklApp {
public:
    scene_manager(): vkl::vklApp("scene_manager"){}
    ~scene_manager() override = default;

private:
    void initDerive() override;
    void drawFrame() override;

private:
    void updateUniformBuffer();
    void loadScene();
    void buildCommands();

private:
    std::shared_ptr<vkl::Light> m_pointLight = nullptr;
    std::shared_ptr<vkl::Light> m_directionalLight = nullptr;
    std::shared_ptr<vkl::Entity> m_model = nullptr;

    std::shared_ptr<vkl::SceneManager> m_sceneManager;
    std::shared_ptr<vkl::SceneRenderer> m_sceneRenderer;
};

#endif // SCENE_MANAGER_H_
