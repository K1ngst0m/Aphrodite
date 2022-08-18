#ifndef LIGHTING_MAPS_H_
#define LIGHTING_MAPS_H_
#include "vklBase.h"
/*
** - https://learnopengl.com/Lighting/Basic-Lighting
 */

struct DescriptorSetLayouts {
    VkDescriptorSetLayout scene;
    VkDescriptorSetLayout material;
};

// per scene data
// general scene data
struct SceneDataLayout {
    glm::vec4 viewPosition;
};

// point light scene data
struct PointLightDataLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

// mvp matrix data layout
struct CameraDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
};

// per material data
struct MaterialDataLayout {
    alignas(16) float     shininess;
};

// per object data
struct ObjectDataLayout {
    glm::mat4 modelMatrix;
};

// vertex data layout
struct VertexDataLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexDataLayout);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

        attributeDescriptions[0] = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexDataLayout, pos),
        };

        attributeDescriptions[1] = {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexDataLayout, normal),
        };

        attributeDescriptions[2] = {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(VertexDataLayout, texCoord),
        };

        return attributeDescriptions;
    }
};


class lighting_maps : public vkl::vkBase {
public:
    lighting_maps();
    ~lighting_maps() override = default;

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
    void createSyncObjects();
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

    Texture m_containerDiffuseTexture;
    Texture m_containerSpecularTexture;

    DescriptorSetLayouts m_descriptorSetLayouts;

    std::vector<VkDescriptorSet> m_perFrameDescriptorSets;
    VkDescriptorSet m_cubeMaterialDescriptorSets;

    VkPipelineLayout m_cubePipelineLayout;
    VkPipeline m_cubeGraphicsPipeline;

    VkPipelineLayout m_emissionPipelineLayout;
    VkPipeline m_emissionGraphicsPipeline;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
};


#endif // LIGHTING_MAPS_H_
