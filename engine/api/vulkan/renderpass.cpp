#include "renderpass.h"

namespace vkl
{
static std::unordered_map<VkSampleCountFlagBits, uint8_t> sampleCountToBitField = {
    { VK_SAMPLE_COUNT_1_BIT, 0 }, { VK_SAMPLE_COUNT_2_BIT, 1 },  { VK_SAMPLE_COUNT_4_BIT, 2 },
    { VK_SAMPLE_COUNT_8_BIT, 3 }, { VK_SAMPLE_COUNT_16_BIT, 4 }, { VK_SAMPLE_COUNT_32_BIT, 5 },
    { VK_SAMPLE_COUNT_64_BIT, 6 }
};

static std::unordered_map<VkImageLayout, uint8_t> imageLayoutToBitField = {
    { VK_IMAGE_LAYOUT_UNDEFINED, 0 },
    { VK_IMAGE_LAYOUT_GENERAL, 1 },
    { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 2 },
    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 3 },
    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 4 },
    { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 5 },
    { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 6 },
    { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 7 },
    { VK_IMAGE_LAYOUT_PREINITIALIZED, 8 },
    { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 9 }
};

}  // namespace vkl
