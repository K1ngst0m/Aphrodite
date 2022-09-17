#ifndef VKLMESH_H_
#define VKLMESH_H_

#include "vklBuffer.h"
#include "vklDevice.h"

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

// vertex data layout
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

struct VertexBuffer : Buffer {
    std::vector<VertexLayout> vertices;
};

struct IndexBuffer : Buffer {
    std::vector<uint32_t> indices;
};

struct UniformBuffer : Buffer {
    void update(const void *data) {
        map();
        copyTo(data, size);
        unmap();
    }

    VkDescriptorBufferInfo getBufferInfo() {
        return descriptorInfo;
    }
};

struct Mesh {
    vkl::VertexBuffer      vertexBuffer;
    vkl::IndexBuffer       indexBuffer;

    void setup(vkl::Device *device, VkQueue transferQueue, std::vector<VertexLayout> vertices = {},
               std::vector<uint32_t> indices = {}, uint32_t vSize = 0, uint32_t iSize = 0);

    VkBuffer getVertexBuffer() const {
        return vertexBuffer.buffer;
    }

    VkBuffer getIndexBuffer() const {
        return indexBuffer.buffer;
    }

    uint32_t getVerticesCount() const {
        return vertexBuffer.vertices.size();
    }

    uint32_t getIndicesCount() const {
        return indexBuffer.indices.size();
    }

    void destroy() const {
        vertexBuffer.destroy();
        indexBuffer.destroy();
    }
};

} // namespace vkl

#endif // VKLMESH_H_
