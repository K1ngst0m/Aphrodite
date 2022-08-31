#include "vklSceneManger.h"

namespace vkl {

void SceneManager::pushUniform(UniformBufferObject *ubo)
{
    _lightNodeList.push_back(new SceneLightNode(ubo));
    // _lightNodeList.emplace_back(ubo);
}
void SceneManager::pushObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform)
{
    _renderNodeList.push_back(new SceneRenderNode(object, pass, &object->_mesh, transform));
}
void SceneManager::drawScene(VkCommandBuffer commandBuffer)
{
    ShaderPass *lastPass = nullptr;
    Mesh * lastMesh = nullptr;
    for (auto *renderNode : _renderNodeList) {

        vkl::DrawContextDirtyBits dirtyBits = DRAWCONTEXT_GLOBAL_SET | DRAWCONTEXT_PUSH_CONSTANT;
        if (!lastPass) {
            dirtyBits = DRAWCONTEXT_ALL;
        } else {
            if (renderNode->_pass->builtPipeline != lastPass->builtPipeline) {
                dirtyBits |= vkl::DRAWCONTEXT_PIPELINE;
            }

            if (lastMesh != renderNode->_mesh){
                dirtyBits |= DRAWCONTEXT_INDEX_BUFFER | DRAWCONTEXT_VERTEX_BUFFER;
            }
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderNode->_pass->layout, 0, 1, &_globalDescriptorSet[0], 0, nullptr);
        renderNode->draw(commandBuffer, dirtyBits);
    }
}
void SceneManager::setupDescriptor(vkl::Device *device, uint32_t setCount, VkDescriptorSetLayout setLayout)
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(setCount * _lightNodeList.size()) },
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = setCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    _globalDescriptorSet.resize(setCount);

    for (auto &set : _globalDescriptorSet) {
        const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, &setLayout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &set));

        std::vector<VkDescriptorBufferInfo> bufferInfos{};

        for (auto *uboNode : _lightNodeList) {
            bufferInfos.push_back(uboNode->_object->buffer.descriptorInfo);
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
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
}
void SceneManager::bindDescriptorSet(VkCommandBuffer commandBuffer, uint32_t setIdx, VkPipelineLayout layout)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &_globalDescriptorSet[setIdx], 0, nullptr);
}
}
