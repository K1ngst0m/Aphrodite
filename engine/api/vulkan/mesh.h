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
};

struct VertexLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;

    VertexLayout() = default;
    VertexLayout(glm::vec3 p, glm::vec2 u)
        :pos(p), normal(glm::vec3(1.0f)), uv(u), color(glm::vec3(1.0f))
    {}
    VertexLayout(glm::vec2 p, glm::vec2 u)
        :pos(glm::vec3(p, 0.0f)), normal(glm::vec3(1.0f)), uv(u), color(glm::vec3(1.0f))
    {}
    VertexLayout(glm::vec3 p, glm::vec3 n, glm::vec2 u, glm::vec3 c = glm::vec3(1.0f))
        :pos(p), normal(n), uv(u), color(c)
    {}
    VertexLayout(glm::vec2 p, glm::vec3 n, glm::vec2 u, glm::vec3 c = glm::vec3(1.0f))
        :pos(glm::vec3(p, 0.0f)), normal(n), uv(u), color(c)
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

inline std::vector<vkl::VertexLayout> planeVertices{
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture
    // wrapping mode). this will cause the floor texture to repeat)
    {{5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},

    {{5.0f, -0.5f, 5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{-5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},
    {{5.0f, -0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f}, {1.0f, 1.0f, 1.0f}},
};

} // namespace vkl

#endif // VKLMESH_H_
