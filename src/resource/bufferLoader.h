#pragma once

#include "api/vulkan/device.h"

namespace aph
{
struct BufferLoadInfo
{
    std::string debugName = {};
    const void* data = {};
    vk::BufferCreateInfo createInfo = {};
};

struct BufferUpdateInfo
{
    const void* data = {};
    Range range = { 0, VK_WHOLE_SIZE };
};
} // namespace aph
