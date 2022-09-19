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
    void keyboardHandleDerive() override;
    void mouseHandleDerive(int xposIn, int yposIn) override;

private:
    virtual void updateUniformBuffer();
    virtual void setupShaders();
    virtual void loadScene();
    virtual void buildCommands();

private:
    vkl::ShaderCache m_shaderCache;

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderEffect m_planeShaderEffect;
    vkl::ShaderPass m_modelShaderPass;
    vkl::ShaderPass m_planeShaderPass;

    vkl::SceneCamera* m_sceneCamera = nullptr;
    vkl::Light* m_pointLight = nullptr;
    vkl::Light* m_directionalLight = nullptr;
    vkl::Entity * m_model = nullptr;
    vkl::Entity * m_plane = nullptr;

    vkl::SceneManager m_sceneManager;
    std::vector<vkl::SceneRenderer *> m_sceneRenderer;
};

#endif // SCENE_MANAGER_H_
