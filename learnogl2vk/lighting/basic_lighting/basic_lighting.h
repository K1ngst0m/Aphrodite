#ifndef BASIC_LIGHTING_H_
#define BASIC_LIGHTING_H_
#include "vklBase.h"

class basic_lighting : public vkl::vklBase {
public:
    ~basic_lighting() override = default;

private:
    void initDerive() override
    {
        createVertexBuffers();
        createUniformBuffers();
        createTextures();
        setupDescriptors();
        createSyncObjects();
        createGraphicsPipeline();
    }

    void drawFrame() override;

    // enable anisotropic filtering features
    void getEnabledFeatures() override;

    void cleanupDerive() override;

private:
    void setupDescriptors()
    {
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();
        createPipelineLayout();
    }
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

    vkl::Texture m_containerTexture;
    vkl::Texture m_awesomeFaceTexture;

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


#endif // BASIC_LIGHTING_H_
