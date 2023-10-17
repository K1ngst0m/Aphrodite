#ifndef APH_GEOMETRY_H_
#define APH_GEOMETRY_H_

#include "api/vulkan/device.h"

namespace aph
{
struct Geometry
{
    static constexpr uint32_t MAX_VERTEX_BINDINGS = 15;

    std::vector<vk::Buffer*> indexBuffer;
    uint32_t                 mIndexType;

    std::vector<vk::Buffer*> vertexBuffers;
    std::vector<uint32_t>    mVertexStrides;

    std::vector<IndirectDrawIndexArguments*> drawArgs;
};
}  // namespace aph

#endif  // APH_GEOMETRY_H_
