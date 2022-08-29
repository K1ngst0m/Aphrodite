#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vklBase.h"

class scene_manager : public vkl::vklBase {
public:
    scene_manager(): vkl::vklBase("extra/scene_manager", 1366, 768){}
    ~scene_manager() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;

private:
    void createDescriptorSetLayouts();
    void createGlobalDescriptorPool();
    void updateUniformBuffer(uint32_t frameIdx);
    void recordCommandBuffer(uint32_t frameIdx);
    void setupShaders();
    void loadScene();
    void setupDescriptorSets();

private:
    enum DescriptorSetType {
        DESCRIPTOR_SET_SCENE,
        DESCRIPTOR_SET_MATERIAL,
        DESCRIPTOR_SET_COUNT
    };

    vkl::ShaderCache m_shaderCache;

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderEffect m_planeShaderEffect;
    vkl::ShaderPass m_modelShaderPass;
    vkl::ShaderPass m_planeShaderPass;

    vkl::Buffer sceneUB;
    vkl::Buffer pointLightUB;
    vkl::Buffer directionalLightUB;
    std::vector<VkDescriptorSet> m_globalDescriptorSet;

    vkl::Model m_model;
    vkl::RenderObject m_planeMesh;
};

#endif // SCENE_MANAGER_H_
