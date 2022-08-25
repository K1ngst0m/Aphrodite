#ifndef LIGHTING_MAPS_H_
#define LIGHTING_MAPS_H_
#include "vklBase.h"

class lighting_maps : public vkl::vklBase {
public:
    ~lighting_maps() override = default;

private:
    void initDerive() override;

    void drawFrame() override;

    // enable anisotropic filtering features
    void getEnabledFeatures() override;

    void cleanupDerive() override;

private:
    void setupDescriptors();
    void createVertexBuffers();
    void createUniformBuffers();
    void createDescriptorSets();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createTextures();
    void createPipelineLayout();

private:
    vkl::Buffer m_cubeVB;

    vkl::Buffer m_sceneUB;
    vkl::Buffer m_pointLightUB;
    vkl::Buffer m_materialUB;

    std::vector<vkl::Buffer> m_mvpUBs;

    vkl::Texture m_containerDiffuseTexture;
    vkl::Texture m_containerSpecularTexture;

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout scene;
        VkDescriptorSetLayout material;
    } m_descriptorSetLayouts;

    std::vector<VkDescriptorSet> m_perFrameDescriptorSets;
    VkDescriptorSet m_cubeMaterialDescriptorSets;

    VkPipelineLayout m_cubePipelineLayout;
    VkPipeline m_cubeGraphicsPipeline;

    VkPipelineLayout m_emissionPipelineLayout;
    VkPipeline m_emissionGraphicsPipeline;
};

#endif // LIGHTING_MAPS_H_
