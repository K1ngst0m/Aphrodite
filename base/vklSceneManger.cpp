#include "vklSceneManger.h"

namespace vkl {

void SceneManager::pushUniform(UniformBufferObject *ubo)
{
    ubolist.push_back(ubo);
}
void SceneManager::pushObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform)
{
    object->setShaderPass(pass);
    object->setupTransform(transform);
    renderList.push_back(object);
}
void SceneManager::drawSceneRecord(VkCommandBuffer commandBuffer, ShaderPass &pass)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pass.layout, 0,
                            1, m_globalDescriptorSet.data(), 0, nullptr);

    ShaderPass *lastPass = nullptr;
    for (auto *renderObj : renderList) {
        vkl::DrawContextDirtyBits dirtyBits;
        if (!lastPass) {
            dirtyBits = DRAWCONTEXT_ALL;
        } else {
            dirtyBits = DRAWCONTEXT_INDEX_BUFFER | DRAWCONTEXT_VERTEX_BUFFER | DRAWCONTEXT_GLOBAL_SET | DRAWCONTEXT_PUSH_CONSTANT;
            ShaderPass *currentPass = renderObj->_pass;
            if (currentPass->builtPipeline != lastPass->builtPipeline) {
                dirtyBits |= vkl::DRAWCONTEXT_PIPELINE;
            }
        }
        renderObj->draw(commandBuffer, dirtyBits);
    }
}
void SceneManager::setupDescriptor(vkl::Device *device, uint32_t setCount, VkDescriptorSetLayout setLayout)
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(setCount * ubolist.size()) },
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = setCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &poolInfo, nullptr, &m_descriptorPool));

    m_globalDescriptorSet.resize(setCount);

    for (auto &set : m_globalDescriptorSet) {
        const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(m_descriptorPool, &setLayout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &set));

        std::vector<VkDescriptorBufferInfo> bufferInfos{};

        for (auto *ubo : ubolist) {
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
void SceneManager::destroy(VkDevice device)
{
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
}
}
