#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "device.h"
#include "vkInit.hpp"
#include "shader.h"

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

class PipelineBuilder {
public:
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

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);

    void setShaders(ShaderEffect *shaders);
    void resetToDefault(VkExtent2D extent);
};
} // namespace vkl

#endif // PIPELINE_H_
