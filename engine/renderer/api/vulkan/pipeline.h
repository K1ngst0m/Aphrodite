#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "device.h"
#include "renderer/gpuResource.h"
#include "vkInit.hpp"
#include "vulkan/vulkan_core.h"

namespace vkl {

enum class VertexComponent {
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

struct VertexInputBuilder {
    static VkVertexInputBindingDescription                _vertexInputBindingDescription;
    static std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    static VkPipelineVertexInputStateCreateInfo           _pipelineVertexInputStateCreateInfo;
    static VkVertexInputAttributeDescription              inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
    static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);
    static void                                           setPipelineVertexInputState(const std::vector<VertexComponent> &components);
};

/**
 * @brief built version of a Shader Effect, where it stores the built pipeline
 */

class VulkanPipelineLayout;

struct PipelineCreateInfo {
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
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
    VkPipelineLayout                             _pipelineLayout;
};

class PipelineBuilder {
public:
    PipelineCreateInfo _createInfo;

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);

    void setShaders(VulkanPipelineLayout *shaders);
    void reset(VkExtent2D extent);
};

class VulkanPipeline : ResourceHandle<VkPipeline> {
public:
    static VulkanPipeline *CreateGraphicsPipeline(VulkanDevice *pDevice, const PipelineCreateInfo * pCreateInfo){
        auto * instance =  new VulkanPipeline;
        return instance;
    }
    static VulkanPipeline *CreateComputePipeline(VulkanDevice *pDevice, const PipelineCreateInfo * pCreateInfo){
        auto * instance =  new VulkanPipeline;
        return instance;
    }

    ~VulkanPipeline(){}

private:
    VulkanDevice* m_device = nullptr;
    VkPipelineBindPoint m_bindPoint;
    std::vector<std::string> m_entryPoints;
};

} // namespace vkl

#endif // PIPELINE_H_
