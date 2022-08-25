#ifndef MODEL_H_
#define MODEL_H_

#include "vklBase.h"

class model : public vkl::vklBase {
public:
    ~model() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;

private:
    void createUniformBuffers();
    void createDescriptorSetLayouts();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void loadModelFromFile(vkl::Model &model, const std::string &path);
    void buildShaderEffect();
    void buildShaderPass();
    void createShaders();
    void loadModel();

private:
    struct ShaderModules{
        VkShaderModule frag;
        VkShaderModule vert;
    } m_shaderModules;

    vklt::ShaderEffect m_modelShaderEffect;
    vklt::ShaderPass m_modelShaderPass;

    struct PerFrameData {
        vkl::Buffer sceneUB;
        vkl::Buffer pointLightUB;
        vkl::Buffer directionalLightUB;
        VkDescriptorSet descriptorSet;
    };
    std::vector<PerFrameData> m_perFrameData;

    vkl::Model m_cubeModel;
};

#endif // MODEL_H_
