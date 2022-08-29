#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "vklBase.h"

namespace vklt{
class SceneManager{
    std::vector<vkl::RenderObject*> renderList;
    std::vector<vkl::UniformBufferObject*> ubolist;
public:
    void pushUniform(vkl::UniformBufferObject* ubo){
        ubolist.push_back(ubo);
    }

    void pushObject(vkl::MeshObject* object, glm::mat4 transform = glm::mat4(1.0f)){
        object->setupTransform(transform);
        renderList.push_back(object);
    }

    void drawSceneRecord(VkCommandBuffer commandBuffer, vkl::ShaderPass & pass){
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pass.layout, 0,
                                1, m_globalDescriptorSet.data(), 0, nullptr);

        for (auto * renderObj : renderList){
            renderObj->draw(commandBuffer, pass.layout);
        }
    }

    void setupDescriptor(vkl::Device * device, uint32_t setCount, VkDescriptorSetLayout setLayout){
        std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(setCount * ubolist.size())},
        };

        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = setCount,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };

        VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));

        m_globalDescriptorSet.resize(setCount);

        for (auto & set : m_globalDescriptorSet) {
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_descriptorPool, &setLayout, 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &set));

            std::vector<VkDescriptorBufferInfo> bufferInfos{};

            for (auto *ubo : ubolist){
                bufferInfos.push_back(ubo->buffer.descriptorInfo);
            }

            std::vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto &bufferInfo : bufferInfos) {
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = set,
                    .dstBinding = static_cast<uint32_t>(descriptorWrites.size()),
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &bufferInfo,
                };
                descriptorWrites.push_back(write);
            }

            vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void destroy(VkDevice device){
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    }

private:
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_globalDescriptorSet;
};
}

class scene_manager : public vkl::vklBase {
public:
    scene_manager(): vkl::vklBase("extra/scene_manager", 1366, 768){}
    ~scene_manager() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;

private:
    void updateUniformBuffer();
    void recordCommandBuffer();
    void setupShaders();
    void loadScene();
    void setupDescriptorSets();

private:
    enum DescriptorSetType {
        DESCRIPTOR_SET_SCENE,
        DESCRIPTOR_SET_MATERIAL,
        DESCRIPTOR_SET_COUNT
    };

    vkl::ShaderCache m_shaderCache;

    vkl::ShaderEffect m_modelShaderEffect;
    vkl::ShaderEffect m_planeShaderEffect;
    vkl::ShaderPass m_modelShaderPass;
    vkl::ShaderPass m_planeShaderPass;

    vkl::UniformBufferObject sceneUBO;
    vkl::UniformBufferObject pointLightUBO;
    vkl::UniformBufferObject directionalLightUBO;

    vkl::Model m_model;
    vkl::MeshObject m_planeMesh;

    vklt::SceneManager m_sceneManager;
};

#endif // SCENE_MANAGER_H_
