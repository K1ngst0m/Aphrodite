#pragma once

#include "api/vulkan/device.h"

namespace aph
{
struct Geometry
{
    static constexpr uint32_t MAX_VERTEX_BINDINGS = 15;

    std::vector<vk::Buffer*> indexBuffer;
    IndexType indexType;

    std::vector<vk::Buffer*> vertexBuffers;
    std::vector<uint32_t> vertexStrides;

    std::vector<DrawIndexArguments*> drawArgs;
};
} // namespace aph
