#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "vklDevice.h"
#include "vklSceneManger.h"

namespace vkl {
class SceneRenderer {
public:
    SceneRenderer(Scene *scene)
        :_scene(scene)
    {}

    virtual void prepareResource() = 0;
    virtual void drawScene()    = 0;

    virtual void destroy() = 0;

    void setScene(Scene *scene) {
        _scene = scene;
        prepareResource();
    }

protected:
    Scene *_scene;
};

class VulkanSceneRenderer final : public SceneRenderer {
public:
    VulkanSceneRenderer(Scene *scene, VkCommandBuffer commandBuffer, vkl::Device *device)
        : SceneRenderer(scene), _drawCmd(commandBuffer), _device(device) {
    }

    void prepareResource() override{
        _setupDescriptor();
    }

    void drawScene() override {
        ShaderPass *lastPass = nullptr;
        Mesh * lastMesh = nullptr;
        for (auto i = 0; i < _opaqueRenderList.size(); i++){
            auto * renderNode = _scene->_opaqueRenderNodeList[i];
            vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderNode->_pass->layout, 0, 1, &_opaqueRenderList[i]._globalDescriptorSet, 0, nullptr);
            renderNode->draw(_drawCmd);
        }

        uint32_t i = _scene->getTransparentRenderableCount() - 1;
        for (auto iter = _scene->_transparentRenderNodeList.rbegin(); iter != _scene->_transparentRenderNodeList.rend(); iter++){
            auto* renderNode = (*iter).second;
            vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderNode->_pass->layout, 0, 1, &_transparentRenderList[i--]._globalDescriptorSet, 0, nullptr);
            renderNode->draw(_drawCmd);
        }
    }

private:
    void _setupDescriptor(){
        std::vector<VkDescriptorPoolSize> poolSizes{
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(_scene->getUBOCount() * _scene->getRenderableCount()) },
        };

        uint32_t maxSetSize = _scene->getRenderableCount();

        for (auto * renderNode : _scene->_opaqueRenderNodeList){
            std::vector<VkDescriptorPoolSize> setInfos = renderNode->_object->getDescriptorSetInfo();
            for (auto& setInfo : setInfos){
                maxSetSize += setInfo.descriptorCount;
                poolSizes.push_back(setInfo);
            }
        }

        for (auto [_, renderNode] : _scene->_transparentRenderNodeList){
            std::vector<VkDescriptorPoolSize> setInfos = renderNode->_object->getDescriptorSetInfo();
            for (auto& setInfo : setInfos){
                maxSetSize += setInfo.descriptorCount;
                poolSizes.push_back(setInfo);
            }
        }

        VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
        VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

        std::vector<VkDescriptorBufferInfo> bufferInfos{};
        if (!_scene->_cameraNodeList.empty()){
            bufferInfos.push_back(_scene->_cameraNodeList[0]->_object->buffer.descriptorInfo);
        }
        for (auto * uboNode : _scene->_uniformNodeList) {
            bufferInfos.push_back(uboNode->_object->buffer.descriptorInfo);
        }

        for (auto & renderNode : _scene->_opaqueRenderNodeList){
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderNode->_pass->effect->setLayouts.data(), 1);
            Renderable renderable;
            VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &renderable._globalDescriptorSet));
            std::vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto &bufferInfo : bufferInfos) {
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = renderable._globalDescriptorSet,
                    .dstBinding = static_cast<uint32_t>(descriptorWrites.size()),
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &bufferInfo,
                };
                descriptorWrites.push_back(write);
            }
            vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

            renderNode->_object->setupDescriptor(renderNode->_pass->effect->setLayouts[1], _descriptorPool);
            _opaqueRenderList.push_back(renderable);
        }

        for (auto [_, renderNode] : _scene->_transparentRenderNodeList){
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderNode->_pass->effect->setLayouts.data(), 1);
            Renderable renderable;
            VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &renderable._globalDescriptorSet));
            std::vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto &bufferInfo : bufferInfos) {
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = renderable._globalDescriptorSet,
                    .dstBinding = static_cast<uint32_t>(descriptorWrites.size()),
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &bufferInfo,
                };
                descriptorWrites.push_back(write);
            }
            vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

            renderNode->_object->setupDescriptor(renderNode->_pass->effect->setLayouts[1], _descriptorPool);
            _transparentRenderList.push_back(renderable);
        }
    }

    void destroy() override{
        vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    }

private:
    VkCommandBuffer _drawCmd;
    vkl::Device    *_device;

    struct Renderable{
        VkDescriptorSet _globalDescriptorSet;
        std::vector<VkDescriptorSet> materialSet;
    };

    std::vector<Renderable> _opaqueRenderList;
    std::vector<Renderable> _transparentRenderList;

    VkDescriptorPool _descriptorPool;
};
} // namespace vkl

#endif // VKLSCENERENDERER_H_
