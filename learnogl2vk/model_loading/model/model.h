#ifndef MODEL_H_
#define MODEL_H_

#include "vklBase.h"

#include <tinygltf/tiny_gltf.h>

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

namespace vkl
{

struct Material {
    glm::vec4 diffuseFactor = glm::vec4(1.0f);
    glm::vec4 specularFactor = glm::vec4(1.0f);
    float shininess = 64.0f;

    vkl::Texture *diffuseTexture = nullptr;
    vkl::Texture *specularTexture = nullptr;

    VkDescriptorSet descriptorSet;

    void createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
    {
        assert(diffuseTexture);
        assert(specularTexture);

        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorSetLayout,
        };
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

        std::vector<VkDescriptorImageInfo> imageDescriptors{};
        std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

        if (diffuseTexture){
            imageDescriptors.push_back(diffuseTexture->descriptorInfo);
            VkWriteDescriptorSet writeDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &diffuseTexture->descriptorInfo,
            };
            writeDescriptorSets.push_back(writeDescriptorSet);
        }

        if (specularTexture){
            imageDescriptors.push_back(specularTexture->descriptorInfo);
            VkWriteDescriptorSet writeDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &specularTexture->descriptorInfo,
            };
            writeDescriptorSets.push_back(writeDescriptorSet);
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
};

struct Model {
    vkl::Mesh _mesh;
    vkl::Material _material;

    void destroy() const{
        _mesh.destroy();
    }
};

}

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
    void createGraphicsPipeline();
    void createSyncObjects();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createTextures();
    void createPipelineLayout();
    void loadModelFromFile(vkl::Model &model, std::string_view path);
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

    vkl::Texture m_containerDiffuseTexture;
    vkl::Texture m_containerSpecularTexture;

    DescriptorSetLayouts m_descriptorSetLayouts;

    VkPipelineLayout m_cubePipelineLayout;
    VkPipeline m_cubeGraphicsPipeline;

    VkPipelineLayout m_emissionPipelineLayout;
    VkPipeline m_emissionGraphicsPipeline;
};

#endif // MODEL_H_
