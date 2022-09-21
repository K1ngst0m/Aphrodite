#ifndef VKLMESH_H_
#define VKLMESH_H_

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

struct VertexLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;

    VertexLayout() = default;
    VertexLayout(glm::vec3 p, glm::vec2 u, glm::vec4 t)
        :pos(p), normal(glm::vec3(1.0f)), uv(u), color(glm::vec3(1.0f)), tangent(t)
    {}
    VertexLayout(glm::vec2 p, glm::vec2 u, glm::vec4 t)
        :pos(glm::vec3(p, 0.0f)), normal(glm::vec3(1.0f)), uv(u), color(glm::vec3(1.0f)), tangent(t)
    {}
    VertexLayout(glm::vec3 p, glm::vec3 n, glm::vec2 u, glm::vec4 t, glm::vec3 c = glm::vec3(1.0f))
        :pos(p), normal(n), uv(u), color(c), tangent(t)
    {}
    VertexLayout(glm::vec2 p, glm::vec3 n, glm::vec2 u, glm::vec4 t, glm::vec3 c = glm::vec3(1.0f))
        :pos(glm::vec3(p, 0.0f)), normal(n), uv(u), color(c), tangent(t)
    {}

    static VkVertexInputBindingDescription                _vertexInputBindingDescription;
    static std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    static VkPipelineVertexInputStateCreateInfo           _pipelineVertexInputStateCreateInfo;

    static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location,
                                                                       VertexComponent component);
    static std::vector<VkVertexInputAttributeDescription>
                inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);
    static void setPipelineVertexInputState(const std::vector<VertexComponent> &components);
};

struct Mesh {
    std::vector<VertexLayout> _vertices;
    vkl::Buffer      vertexBuffer;

    std::vector<uint32_t> _indices;
    vkl::Buffer       indexBuffer;

    void setup(vkl::Device *device, VkQueue transferQueue, std::vector<VertexLayout> vertices = {},
               std::vector<uint32_t> indices = {}, uint32_t vSize = 0, uint32_t iSize = 0);

    VkBuffer getVertexBuffer() const;

    VkBuffer getIndexBuffer() const;

    uint32_t getVerticesCount() const;

    uint32_t getIndicesCount() const;

    void destroy() const;
};

} // namespace vkl

#endif // VKLMESH_H_
