#ifndef BLENDING_H_
#define BLENDING_H_

#include "vklBase.h"

class framebuffers : public vkl::vklBase {
public:
    framebuffers(): vkl::vklBase("advance/framebuffers", 1366, 768){}
    ~framebuffers() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;

private:
    void prepareAttachmentResouces();
    void prepareRenderPass();
    void prepareFramebuffers();
    void updateUniformBuffer();
    void setupShaders();
    void loadScene();
    void buildCommands();

private:
    vkl::ShaderCache m_shaderCache;

    struct {
        std::vector<vkl::Texture> colorAttachments;
        vkl::Texture depthAttachment;
        std::vector<VkFramebuffer> framebuffers;
        VkRenderPass renderPass;

        vkl::ShaderEffect shaderEffect;
        vkl::ShaderPass shaderPass;

        void build(vkl::Device * device, vkl::PipelineBuilder & pipelineBuilder){
            shaderPass.build(device->logicalDevice, renderPass, pipelineBuilder, &shaderEffect);
        }

        void destory(vkl::Device * device){
            depthAttachment.destroy();
            for (auto & attachment: colorAttachments){
                attachment.destroy();
            }
            for (auto & framebuffer : framebuffers){
                vkDestroyFramebuffer(device->logicalDevice, framebuffer, nullptr);
            }
            vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
            shaderPass.destroy(device->logicalDevice);
            shaderEffect.destroy(device->logicalDevice);
        }
    } m_offscreenPass;

    struct {
        std::vector<VkFramebuffer> framebuffers;
        VkRenderPass renderPass;

        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        vkl::ShaderEffect shaderEffect;
        vkl::ShaderPass shaderPass;

        void build(vkl::Device * device, vkl::PipelineBuilder & pipelineBuilder){
            shaderPass.build(device->logicalDevice, renderPass, pipelineBuilder, &shaderEffect);
        }

        void destory(vkl::Device * device){
            for (auto & framebuffer : framebuffers){
                vkDestroyFramebuffer(device->logicalDevice, framebuffer, nullptr);
            }
            vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
            vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
            shaderPass.destroy(device->logicalDevice);
            shaderEffect.destroy(device->logicalDevice);
        }
    } m_postProcessPass;

    vkl::UniformBufferObject m_sceneUBO;
    vkl::MeshObject m_cubeMesh;
    vkl::MeshObject m_planeMesh;
    vkl::MeshObject m_quadMesh;

    vkl::Scene m_defaultScene;
};

#endif // BLENDING_H_
