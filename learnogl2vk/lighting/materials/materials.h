#ifndef MATERIALS_H_
#define MATERIALS_H_
#include "vklBase.h"

class materials : public vkl::vklBase {
public:
    materials();
    ~materials() override = default;

private:
    void initDerive() override;

    void drawFrame() override;

    // enable anisotropic filtering features
    void getEnabledFeatures() override;

    void keyboardHandleDerive() override;

    void mouseHandleDerive(int xposIn, int yposIn) override;

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
    void recordCommandBuffer();
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


#endif // MATERIALS_H_
