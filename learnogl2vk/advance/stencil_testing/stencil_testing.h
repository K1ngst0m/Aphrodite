#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vklBase.h"

class stencil_testing : public vkl::vklBase {
public:
    stencil_testing(): vkl::vklBase("advance/stencil_testing", 1366, 768){}
    ~stencil_testing() override = default;

private:
    void initDerive() override;
    void drawFrame() override;

private:
    void loadScene();
    void updateUniformBuffer();
    void setupShaders();
    void buildCommands();

private:
    vkl::ShaderCache m_shaderCache;

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderPass m_modelShaderPass;

    vkl::ShaderEffect m_outlineShaderEffect;
    vkl::ShaderPass m_outlineShaderPass;

    vkl::UniformBufferObject sceneUBO;
    vkl::UniformBufferObject pointLightUBO;
    vkl::UniformBufferObject directionalLightUBO;

    vkl::Model m_model;

    vkl::Scene m_defaultScene;
};

#endif // SCENE_MANAGER_H_
