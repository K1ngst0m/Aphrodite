#ifndef DEPTH_TESTING_H_
#define DEPTH_TESTING_H_

#include "vklBase.h"

class depth_testing : public vkl::vklBase {
public:
    ~depth_testing() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;
    void keyboardHandleDerive() override;

private:
    void setupDescriptors();
    void createUniformBuffers();
    void createDescriptorSets();
    void createDescriptorSetLayout();
    void setupPipelineBuilder();
    void createGraphicsPipeline();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer();
    void createPipelineLayout();
    void loadScene();

private:
    struct PerFrameData{
        vkl::Buffer cameraUB;
        vkl::Buffer pointLightUB;
        vkl::Buffer directionalLightUB;
        VkDescriptorSet cameraDescriptorSet;
        VkDescriptorSet sceneDescriptorSet;
    };
    std::vector<PerFrameData> m_perFrameData;

    vkl::Model m_planeModel;
    vkl::Model m_cubeModel;

    bool enabledDepthVisualizing = false;

    struct{
        VkDescriptorSetLayout camera;
        VkDescriptorSetLayout scene;
        VkDescriptorSetLayout material;
    } m_descriptorSetLayouts;

    struct{
        VkPipelineLayout model;
        VkPipelineLayout depth;
    } m_pipelineLayouts;

    struct{
        VkPipeline model;
        VkPipeline depth;
    } m_pipelines;
};

#endif // DEPTH_TESTING_H_
