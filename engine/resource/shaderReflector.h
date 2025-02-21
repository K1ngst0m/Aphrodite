#pragma once

#include "api/vulkan/shader.h"

namespace aph
{
vk::ResourceLayout         reflectLayout(const std::vector<uint32_t>& spvCode);
vk::CombinedResourceLayout combineLayout(const std::vector<vk::Shader*>& shaders,
                                         const vk::ImmutableSamplerBank* samplerBank = nullptr);
VertexInput                getVertexInputInfo(vk::CombinedResourceLayout combineLayout);
}  // namespace aph
