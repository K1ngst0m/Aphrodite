#ifndef MODEL_H_
#define MODEL_H_

#include "vklBase.h"
#include "vklModel.h"

struct DescriptorSetLayouts {
    VkDescriptorSetLayout scene;
    VkDescriptorSetLayout material;
};

// per scene data
// general scene data
struct SceneDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 viewPosition;
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

// per object data
struct ObjectDataLayout {
    glm::mat4 modelMatrix;
};

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
    void loadModelFromFile(vkl::Model &model, std::string path);
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

    DescriptorSetLayouts m_descriptorSetLayouts;

    vkl::utils::PipelineBuilder m_pipelineBuilder;

    VkPipelineLayout m_modelPipelineLayout;
    VkPipeline m_modelGraphicsPipeline;
};

#endif // MODEL_H_
