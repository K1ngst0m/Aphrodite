#ifndef MODEL_H_
#define MODEL_H_

#include "vklBase.h"

namespace vklt {
struct Transfrom {
    glm::vec3 pos {0.0f};
    glm::vec3 eulerRot {0.0f};
    glm::vec3 scale {1.0f};

    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct Entry{

};

class SceneManager{

};
}

class model : public vkl::vklBase {
public:
    model(): vkl::vklBase("model_loading/model"){}
    ~model() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;

private:
    void createUniformBuffers();
    void createDescriptorSetLayouts();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void setupPipelineBuilder();
    void setupShaders();
    void loadScene();
    void setupDescriptorSets();

private:
    enum DescriptorSetType {
        DESCRIPTOR_SET_SCENE,
        DESCRIPTOR_SET_MATERIAL,
        DESCRIPTOR_SET_COUNT
    };

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderPass m_modelShaderPass;

    struct PerFrameData {
        vkl::Buffer sceneUB;
        vkl::Buffer pointLightUB;
        vkl::Buffer directionalLightUB;
        VkDescriptorSet descriptorSet;
    };
    std::vector<PerFrameData> m_perFrameData;

    vkl::Model m_cubeModel;
};

#endif // MODEL_H_
