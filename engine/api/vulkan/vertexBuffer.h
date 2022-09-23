#ifndef VERTEXBUFFER_H_
#define VERTEXBUFFER_H_

#include "buffer.h"
#include "device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <vulkan/vulkan.h>

namespace vkl {

enum class VertexComponent {
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

struct VertexInputBuilder{
    static VkVertexInputBindingDescription                _vertexInputBindingDescription;
    static std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    static VkPipelineVertexInputStateCreateInfo           _pipelineVertexInputStateCreateInfo;
    static VkVertexInputAttributeDescription              inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
    static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);
    static void                                           setPipelineVertexInputState(const std::vector<VertexComponent> &components);
};


} // namespace vkl

#endif // VERTEXBUFFER_H_
