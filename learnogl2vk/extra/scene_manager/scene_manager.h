#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vklBase.h"
#include "vklSceneRenderer.h"

class scene_manager : public vkl::vklBase {
public:
    scene_manager(): vkl::vklBase("extra/scene_manager", 1366, 768){}
    ~scene_manager() override = default;

private:
    void initDerive() override;
    void drawFrame() override;

private:
    void updateUniformBuffer();
    void setupShaders();
    void loadScene();
    void buildCommands();

private:
    vkl::ShaderCache m_shaderCache;

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderEffect m_planeShaderEffect;
    vkl::ShaderPass m_modelShaderPass;
    vkl::ShaderPass m_planeShaderPass;

    vkl::UniformBufferObject* sceneUBO;
    vkl::UniformBufferObject* pointLightUBO;
    vkl::UniformBufferObject* directionalLightUBO;

    vkl::Entity * m_model;
    vkl::Entity * m_plane;

    vkl::SceneManager m_sceneManager;
    std::vector<vkl::SceneRenderer *> m_sceneRenderer;
};

#endif // SCENE_MANAGER_H_
