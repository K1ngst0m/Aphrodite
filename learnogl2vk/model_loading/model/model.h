#ifndef MESH_H_
#define MESH_H_

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

// flash light data
struct FlashLightDataLayout {
    glm::vec4 position;
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    alignas(4) float cutOff;
    alignas(4) float outerCutOff;
};

// point light scene data
struct DirectionalLightDataLayout {
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

// point light scene data
struct PointLightDataLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 attenuationFactor;
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

struct Model{
    vkl::Mesh _mesh;

    void loadFromFile(const std::filesystem::path& path){

    }
};

class model_loading : public vkl::vklBase {
public:
    ~model_loading() override = default;

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
    void createGraphicsPipeline();
    void createSyncObjects();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createTextures();
    void createPipelineLayout();
    void loadModel();

private:
    vkl::Mesh   m_cubeMesh;

    vkl::Buffer m_sceneUB;

    vkl::Buffer m_pointLightUB;
    vkl::Buffer m_flashLightUB;
    vkl::Buffer m_directionalLightUB;

    vkl::Buffer m_materialUB;

    std::vector<vkl::Buffer> m_mvpUBs;

    vkl::Texture m_containerDiffuseTexture;
    vkl::Texture m_containerSpecularTexture;

    DescriptorSetLayouts m_descriptorSetLayouts;

    std::vector<VkDescriptorSet> m_perFrameDescriptorSets;
    VkDescriptorSet m_cubeMaterialDescriptorSets;

    VkPipelineLayout m_cubePipelineLayout;
    VkPipeline m_cubeGraphicsPipeline;

    VkPipelineLayout m_emissionPipelineLayout;
    VkPipeline m_emissionGraphicsPipeline;
};

#endif // MESH_H_
