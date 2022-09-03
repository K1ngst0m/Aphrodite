#ifndef BLENDING_H_
#define BLENDING_H_

#include "vklBase.h"

class blending : public vkl::vklBase {
public:
    blending(): vkl::vklBase("advance/blending", 1366, 768){}
    ~blending() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;

private:
    void updateUniformBuffer();
    void setupShaders();
    void loadScene();
    void buildCommands();

private:
    vkl::ShaderCache m_shaderCache;

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderEffect m_planeShaderEffect;
    vkl::ShaderEffect m_depthShaderEffect;

    vkl::ShaderPass m_modelShaderPass;
    vkl::ShaderPass m_planeShaderPass;
    vkl::ShaderPass m_depthShaderPass;

    vkl::UniformBufferObject m_sceneUBO;
    vkl::UniformBufferObject m_pointLightUBO;
    vkl::UniformBufferObject m_directionalLightUBO;

    vkl::Model m_model;
    vkl::MeshObject m_planeMesh;

    vkl::Scene m_defaultScene;

    vkl::Scene m_depthScene;

    bool enableDepthVisualization = false;
};

#endif // BLENDING_H_
