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
        _initRenderList();
        _setupDescriptor();
    }

    void drawScene() override {
        for (auto & renderable : _renderList){
            vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderable.shaderPass->layout, 0, 1, &renderable.globalDescriptorSet, 0, nullptr);
            renderable.draw(_drawCmd);
        }
    }

    void destroy() override{
        vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    }

private:
    void _initRenderList(){
        for (auto * renderNode : _scene->_renderNodeList){

            Renderable renderable;
            renderable.shaderPass = renderNode->_pass;
            renderable.object = renderNode->_object;
            renderable.transform = renderNode->_transform;

            _renderList.push_back(renderable);
        }
    }

    void _setupDescriptor(){
        std::vector<VkDescriptorPoolSize> poolSizes{
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(_scene->getUBOCount() * _scene->getRenderableCount()) },
        };

        uint32_t maxSetSize = _scene->getRenderableCount();

        for (auto * renderNode : _scene->_renderNodeList){
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

        for (size_t i = 0; i < _renderList.size(); i++){
            auto * renderNode = _scene->_renderNodeList[i];
            const VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderNode->_pass->effect->setLayouts.data(), 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &_renderList[i].globalDescriptorSet));
            std::vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto &bufferInfo : bufferInfos) {
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = _renderList[i].globalDescriptorSet,
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
        }
    }

private:
    VkCommandBuffer _drawCmd;
    vkl::Device    *_device;

    struct Renderable{
        VkDescriptorSet globalDescriptorSet;
        std::vector<VkDescriptorSet> materialSet;
        vkl::RenderObject * object;
        glm::mat4 transform;

        vkl::ShaderPass * shaderPass;

        void draw(VkCommandBuffer commandBuffer) const{
            object->draw(commandBuffer, shaderPass, transform);
        }
    };

    std::vector<Renderable> _renderList;

    VkDescriptorPool _descriptorPool;
};
} // namespace vkl

#endif // VKLSCENERENDERER_H_
