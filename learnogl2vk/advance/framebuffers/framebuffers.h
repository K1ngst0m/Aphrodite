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
    void prepareOffscreen();
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

        void prepareAttachmentResources(vkl::Device* device, VkQueue queue, uint32_t attachmentCount, VkExtent2D extent){
            // offscreen - color
            {
                colorAttachments.resize(attachmentCount);
                for (auto & attachment: colorAttachments){
                    device->createImage(extent.width, extent.height, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                        attachment);
                    attachment.view = device->createImageView(attachment.image, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

                    VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
                    VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &attachment.sampler));
                    attachment.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }

            // offscreen - depth
            {
                VkFormat depthFormat = device->findDepthFormat();
                device->createImage(extent.width, extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    depthAttachment);
                depthAttachment.view = device->createImageView(depthAttachment.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
                device->transitionImageLayout(queue, depthAttachment.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            }
        }

        void prepareRenderPass(vkl::Device * device, VkFormat format){
            VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,};
            VkAttachmentReference depthAttachmentRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,};

            VkAttachmentDescription depthAttachment{
                .format         = device->findDepthFormat(),
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            };

            VkAttachmentDescription colorAttachment{
                .format         = format,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkSubpassDescription subpass{
                .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount    = 1,
                .pColorAttachments       = &colorAttachmentRef,
                .pDepthStencilAttachment = &depthAttachmentRef,
            };

            std::array<VkSubpassDependency, 2> dependencies;

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
            VkRenderPassCreateInfo                 renderPassInfo{
                                .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                                .pAttachments    = attachments.data(),
                                .subpassCount    = 1,
                                .pSubpasses      = &subpass,
                                .dependencyCount = dependencies.size(),
                                .pDependencies   = dependencies.data(),
            };

            VK_CHECK_RESULT(vkCreateRenderPass(device->logicalDevice, &renderPassInfo, nullptr, &renderPass));
        }

        void prepareFramebuffers(vkl::Device* device, uint32_t imageCount, VkExtent2D extent){
            framebuffers.resize(imageCount);
            for (size_t i = 0; i < imageCount; i++) {
                std::array<VkImageView, 2> attachments = {colorAttachments[i].view,
                                                          depthAttachment.view};

                VkFramebufferCreateInfo framebufferInfo{
                    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass      = renderPass,
                    .attachmentCount = static_cast<uint32_t>(attachments.size()),
                    .pAttachments    = attachments.data(),
                    .width           = extent.width,
                    .height          = extent.height,
                    .layers          = 1,
                };

                VK_CHECK_RESULT(vkCreateFramebuffer(device->logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]));
            }
        }

        void build(vkl::Device * device, vkl::PipelineBuilder & pipelineBuilder){
            shaderPass.build(device->logicalDevice, renderPass, pipelineBuilder, &shaderEffect);
        }

        void destroy(vkl::Device * device){
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
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        vkl::ShaderEffect shaderEffect;
        vkl::ShaderPass shaderPass;

        void build(vkl::Device * device, VkRenderPass renderPass, vkl::PipelineBuilder & pipelineBuilder){
            shaderPass.build(device->logicalDevice, renderPass, pipelineBuilder, &shaderEffect);
        }

        void destroy(vkl::Device * device){
            vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
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
