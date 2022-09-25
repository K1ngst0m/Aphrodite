#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#define VKL_SESSION_USING_SCENE_MANAGER

#include "app.h"

class scene_manager : public vkl::vklApp {
public:
    scene_manager(): vkl::vklApp("scene_manager", 1366, 768){}
    ~scene_manager() override = default;

private:
    void initDerive() override;
    void drawFrame() override;

private:
    virtual void updateUniformBuffer();
    virtual void loadScene();
    virtual void buildCommands();

private:
    std::shared_ptr<vkl::Light> m_pointLight = nullptr;
    std::shared_ptr<vkl::Light> m_directionalLight = nullptr;
    std::shared_ptr<vkl::Entity> m_model = nullptr;

    vkl::SceneManager m_sceneManager;
    std::vector<std::shared_ptr<vkl::SceneRenderer>> m_sceneRenderer;
};

#endif // SCENE_MANAGER_H_
