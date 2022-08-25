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
    void setupPipelineBuilder();
    void createDescriptorSetLayouts();
    void createGraphicsPipeline();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void loadModelFromFile(vkl::Model &model, const std::string &path);
    void createShaders();
    void loadModel();

private:
    struct ShaderModules{
        VkShaderModule frag;
        VkShaderModule vert;
    } m_shaderModules;

    vklt::ShaderEffect m_modelShader;

    struct PerFrameData {
        vkl::Buffer sceneUB;
        vkl::Buffer pointLightUB;
        vkl::Buffer directionalLightUB;
        VkDescriptorSet descriptorSet;
    };
    std::vector<PerFrameData> m_perFrameData;

    vkl::Model m_cubeModel;

    vklt::PipelineBuilder m_pipelineBuilder;

    VkPipeline m_modelGraphicsPipeline;
};

#endif // MODEL_H_
