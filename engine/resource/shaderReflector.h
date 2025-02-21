#pragma once

#include "api/vulkan/shader.h"

namespace aph
{
vk::ResourceLayout ReflectLayout(const std::vector<uint32_t>& spvCode);
}
