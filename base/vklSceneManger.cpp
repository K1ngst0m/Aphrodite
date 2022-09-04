#include "vklSceneManger.h"

namespace vkl {
Scene &Scene::pushCamera(vkl::Camera *camera, UniformBufferObject *ubo) {
    _cameraNodeList.push_back(new SceneCameraNode(ubo, camera));
    return *this;
}

Scene& Scene::pushUniform(UniformBufferObject *ubo)
{
    _uniformNodeList.push_back(new SceneUniformNode(ubo));
    return *this;
}

Scene& Scene::pushObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform, SCENE_RENDER_TYPE renderType)
{
    if (renderType == SCENE_RENDER_TYPE::TRANSPARENCY){
        float distance = glm::length(_cameraNodeList[0]->_camera->m_position - glm::vec3(transform * glm::vec4({0.0f, 0.0f, 0.0f, 1.0f})));
        _transparentRenderNodeList[distance] = (new SceneRenderNode(object, pass, &object->_mesh, transform));
    }
    else if(renderType == SCENE_RENDER_TYPE::OPAQUE){
        _opaqueRenderNodeList.push_back(new SceneRenderNode(object, pass, &object->_mesh, transform));
    }
    return *this;
}

void Scene::draw(VkCommandBuffer commandBuffer)
{
    ShaderPass *lastPass = nullptr;
    Mesh * lastMesh = nullptr;
    for (auto *renderNode : _opaqueRenderNodeList) {
        vkl::DrawContextDirtyBits dirtyBits = DRAWCONTEXT_GLOBAL_SET | DRAWCONTEXT_PUSH_CONSTANT;
        if (!lastPass) {
            dirtyBits = DRAWCONTEXT_ALL;
        }
        else {
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

    for (auto iter = _transparentRenderNodeList.rbegin();
         iter != _transparentRenderNodeList.rend(); iter++){
        auto renderNode = (*iter).second;
        vkl::DrawContextDirtyBits dirtyBits = DRAWCONTEXT_GLOBAL_SET | DRAWCONTEXT_PUSH_CONSTANT;
        if (!lastPass) {
            dirtyBits = DRAWCONTEXT_ALL;
        }
        else {
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
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>((_uniformNodeList.size() + _cameraNodeList.size()) * (_opaqueRenderNodeList.size() + _transparentRenderNodeList.size())) },
    };

    uint32_t maxSetSize = _opaqueRenderNodeList.size() + _transparentRenderNodeList.size();

    for (auto * renderNode : _opaqueRenderNodeList){
        std::vector<VkDescriptorPoolSize> setInfos = renderNode->_object->getDescriptorSetInfo();
        for (auto& setInfo : setInfos){
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    for (auto [_, renderNode] : _transparentRenderNodeList){
        std::vector<VkDescriptorPoolSize> setInfos = renderNode->_object->getDescriptorSetInfo();
        for (auto& setInfo : setInfos){
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool));

    std::vector<VkDescriptorBufferInfo> bufferInfos{};
    if (!_cameraNodeList.empty()){
        bufferInfos.push_back(_cameraNodeList[0]->_object->buffer.descriptorInfo);
    }
    for (auto uboNode : _uniformNodeList) {
        bufferInfos.push_back(uboNode->_object->buffer.descriptorInfo);
    }

    for (auto & renderNode : _opaqueRenderNodeList){
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

        renderNode->_object->setupDescriptor(renderNode->_pass->effect->setLayouts[1], _descriptorPool);
    }

    for (auto [_, renderNode] : _transparentRenderNodeList){
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

        renderNode->_object->setupDescriptor(renderNode->_pass->effect->setLayouts[1], _descriptorPool);
    }
}

void Scene::destroy(VkDevice device)
{
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
}
} // namespace vkl
