#ifndef BLENDING_H_
#define BLENDING_H_

#include "vklBase.h"

class face_culling : public vkl::vklBase {
public:
    face_culling(): vkl::vklBase("advance/face_culling", 1366, 768){}
    ~face_culling() override = default;

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

    vkl::Scene m_defaultScene;
};

#endif // BLENDING_H_
