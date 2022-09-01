#include "vklSceneManger.h"

namespace vkl {

Scene& Scene::pushUniform(UniformBufferObject *ubo)
{
    _lightNodeList.push_back(new SceneLightNode(ubo));
    return *this;
}
Scene& Scene::pushObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform)
{
    _renderNodeList.push_back(new SceneRenderNode(object, pass, &object->_mesh, transform));
    return *this;
}
void Scene::drawScene(VkCommandBuffer commandBuffer)
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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderNode->_pass->layout, 0, 1, &renderNode->_globalDescriptorSet, 0, nullptr);
        renderNode->draw(commandBuffer, dirtyBits);
    }
}
void Scene::setupDescriptor(VkDevice device)
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(_lightNodeList.size() * _renderNodeList.size()) },
    };
    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, _renderNodeList.size());
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool));

    std::vector<VkDescriptorBufferInfo> bufferInfos{};
    for (auto *uboNode : _lightNodeList) {
        bufferInfos.push_back(uboNode->_object->buffer.descriptorInfo);
    }

    for (auto & renderNode : _renderNodeList){
        const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderNode->_pass->effect->setLayouts.data(), 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &renderNode->_globalDescriptorSet));
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto &bufferInfo : bufferInfos) {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = renderNode->_globalDescriptorSet,
                .dstBinding = static_cast<uint32_t>(descriptorWrites.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo,
            };
            descriptorWrites.push_back(write);
        }
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        renderNode->_object->setupDescriptor(renderNode->_pass->effect->setLayouts[1]);
    }

}
void Scene::destroy(VkDevice device)
{
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
}
}
