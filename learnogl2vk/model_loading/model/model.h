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
    void setupDescriptors();
    void createUniformBuffers();
    void createDescriptorSets();
    void createDescriptorSetLayout();
    void setupPipelineBuilder();
    void createGraphicsPipeline();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createPipelineLayout();
    void loadModelFromFile(vkl::Model &model, const std::string& path);
    void loadModel();

private:
    struct PerFrameData{
        vkl::Buffer sceneUB;
        vkl::Buffer pointLightUB;
        vkl::Buffer directionalLightUB;
        VkDescriptorSet descriptorSet;
    };
    std::vector<PerFrameData> m_perFrameData;

    vkl::Model m_cubeModel;

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout scene;
        VkDescriptorSetLayout material;
    } m_descriptorSetLayouts;

    vkl::utils::PipelineBuilder m_pipelineBuilder;

    VkPipelineLayout m_modelPipelineLayout;
    VkPipeline m_modelGraphicsPipeline;
};

#endif // MODEL_H_
