#ifndef DEPTH_TESTING_H_
#define DEPTH_TESTING_H_

#include "vklBase.h"

class depth_testing : public vkl::vklBase {
public:
    depth_testing(): vkl::vklBase("advance/depth_testing", 1366, 768){}
    ~depth_testing() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;

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

    vkl::UniformBufferObject sceneUBO;
    vkl::UniformBufferObject pointLightUBO;
    vkl::UniformBufferObject directionalLightUBO;

    vkl::Model m_model;
    vkl::MeshObject m_planeMesh;

    vkl::Scene m_defaultScene;

    vkl::Scene m_depthScene;

    bool enableDepthVisualization = false;
};

#endif // DEPTH_TESTING_H_
