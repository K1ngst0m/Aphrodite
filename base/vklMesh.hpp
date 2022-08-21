#ifndef VKLMESH_H_
#define VKLMESH_H_

#include "vklBuffer.h"
#include "vklTexture.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include <vector>

namespace vkl {
// vertex data layout
struct VertexDataLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexDataLayout);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

        attributeDescriptions[0] = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexDataLayout, pos),
        };

        attributeDescriptions[1] = {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexDataLayout, normal),
        };

        attributeDescriptions[2] = {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(VertexDataLayout, texCoord),
        };

        return attributeDescriptions;
    }
};

struct Mesh {
    std::vector<VertexDataLayout>  vertices;
    std::vector<uint32_t>      indices;
    // std::vector<vkl::Texture>      textures;

    vkl::Buffer vertexBuffer;
    vkl::Buffer indexBuffer;

    void destroy() const{
        vertexBuffer.destroy();
        indexBuffer.destroy();

    }
};
}


#endif // VKLMESH_H_
