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

    vkl::ShaderEffect m_defaultShaderEffect;
    vkl::ShaderPass m_defaultShaderPass;

    vkl::UniformBufferObject m_sceneUBO;
    vkl::MeshObject m_cubeMesh;
    vkl::MeshObject m_transparentMesh;
    vkl::MeshObject m_planeMesh;

    vkl::Scene m_defaultScene;
};

#endif // BLENDING_H_
