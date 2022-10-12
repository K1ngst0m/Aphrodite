#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "device.h"
#include "renderer/gpuResource.h"
#include "vkInit.hpp"

namespace vkl {

enum class VertexComponent {
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

struct VertexInputBuilder {
    VkVertexInputBindingDescription                _vertexInputBindingDescription;
    std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo           _pipelineVertexInputStateCreateInfo;
    VkVertexInputAttributeDescription              inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);
    VkPipelineVertexInputStateCreateInfo           getPipelineVertexInputState(const std::vector<VertexComponent> &components);
};

struct PipelineCreateInfo {
    std::vector<VkDynamicState>                  _dynamicStages;
    VkPipelineVertexInputStateCreateInfo         _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo       _inputAssembly;
    VkViewport                                   _viewport;
    VkRect2D                                     _scissor;
    VkPipelineDynamicStateCreateInfo             _dynamicState;
    VkPipelineRasterizationStateCreateInfo       _rasterizer;
    VkPipelineColorBlendAttachmentState          _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo         _multisampling;
    VkPipelineDepthStencilStateCreateInfo        _depthStencil;
    VertexInputBuilder                           _vertexInputBuilder;

    PipelineCreateInfo(VkExtent2D extent = {0, 0}) {
        _vertexInputInfo = _vertexInputBuilder.getPipelineVertexInputState({VertexComponent::POSITION,
                                                                            VertexComponent::NORMAL,
                                                                            VertexComponent::UV,
                                                                            VertexComponent::COLOR,
                                                                            VertexComponent::TANGENT});
        _vertexInputInfo = _vertexInputBuilder._pipelineVertexInputStateCreateInfo;

        _inputAssembly = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        _dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        _dynamicState  = vkl::init::pipelineDynamicStateCreateInfo(_dynamicStages.data(), static_cast<uint32_t>(_dynamicStages.size()));

        _rasterizer           = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        _multisampling        = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        _colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        _depthStencil         = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    }
};

class VulkanPipeline : public ResourceHandle<VkPipeline> {
public:
    static VulkanPipeline *CreateGraphicsPipeline(VulkanDevice *pDevice,
                                                  const PipelineCreateInfo *pCreateInfo,
                                                  ShaderEffect * effect,
                                                  VulkanRenderPass * pRenderPass);

    static VulkanPipeline *CreateComputePipeline(VulkanDevice *pDevice, const PipelineCreateInfo *pCreateInfo);

    ~VulkanPipeline() {
        vkDestroyPipeline(_device->getHandle(), _handle, nullptr);
    }

    VkPipelineLayout getPipelineLayout();
    VkDescriptorSetLayout* getDescriptorSetLayout(uint32_t idx);

private:
    PipelineCreateInfo       _createInfo;
    VulkanDevice            *_device = nullptr;
    ShaderEffect            *_effect = nullptr;
};

} // namespace vkl

#endif // PIPELINE_H_
