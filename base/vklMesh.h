#ifndef VKLMESH_H_
#define VKLMESH_H_

#include "vklBuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include <vector>

namespace vkl {

// vertex data component
enum class VertexComponent {POSITION, NORMAL, UV, COLOR};

// vertex data layout
struct VertexLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;

    static VkVertexInputBindingDescription _vertexInputBindingDescription;
    static std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    static VkPipelineVertexInputStateCreateInfo _pipelineVertexInputStateCreateInfo;

    static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
    static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);
    static void setPipelineVertexInputState(const std::vector<VertexComponent> &components);
};


struct VertexBuffer : Buffer{
    std::vector<VertexLayout> vertices;
};

struct IndexBuffer : Buffer{
    std::vector<uint32_t> indices;
};

struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t materialIndex;
};

struct Mesh {
    vkl::VertexBuffer vertexBuffer;
    vkl::IndexBuffer indexBuffer;
    std::vector<Primitive> primitives;

    uint32_t getIndicesCount() const{
        return indexBuffer.indices.size();
    }

    void destroy() const{
        vertexBuffer.destroy();
        indexBuffer.destroy();
    }
};

}


#endif // VKLMESH_H_
