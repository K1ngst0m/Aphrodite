#include "parameterLayout.h"
#include "common/logger.h"

namespace aph
{
auto ParameterLayout::generateLayout(const MaterialTemplate* pTemplate) -> SmallVector<MaterialParameterDesc>
{
    if (!pTemplate)
    {
        APH_LOG_ERR("Cannot generate parameter layout: null template provided");
        return {};
    }

    const auto& parameters = pTemplate->getParameterLayout();

    SmallVector<MaterialParameterDesc> uniformParams;
    SmallVector<MaterialParameterDesc> textureParams;

    separateParameters(parameters, uniformParams, textureParams);

    SmallVector<MaterialParameterDesc> alignedLayout;

    // Process uniform parameters first (these go into the uniform buffer)
    uint32_t currentOffset = 0;

    for (const auto& param : uniformParams)
    {
        MaterialParameterDesc alignedParam = param;
        alignedParam.offset                = calculateAlignedOffset(currentOffset, param.type);
        currentOffset                      = alignedParam.offset + alignedParam.size;

        alignedLayout.push_back(alignedParam);
    }

    // Process texture parameters (these are bound as descriptors)
    currentOffset = 0;

    for (const auto& param : textureParams)
    {
        MaterialParameterDesc alignedParam = param;
        alignedParam.offset                = currentOffset;
        currentOffset += alignedParam.size;

        alignedLayout.push_back(alignedParam);
    }

    return alignedLayout;
}

auto ParameterLayout::calculateAlignedOffset(uint32_t currentOffset, DataType type) -> uint32_t
{
    uint32_t alignment = TypeUtils::getTypeAlignment(type);
    return (currentOffset + alignment - 1) & ~(alignment - 1);
}

auto ParameterLayout::calculateTotalSize(const SmallVector<MaterialParameterDesc>& parameters) -> uint32_t
{
    if (parameters.empty())
    {
        return 0;
    }

    uint32_t maxEnd = 0;

    for (const auto& param : parameters)
    {
        uint32_t end = param.offset + param.size;
        maxEnd       = std::max(maxEnd, end);
    }

    // Ensure the total size is aligned to a proper boundary for UBO requirements
    constexpr uint32_t UBO_ALIGNMENT = 16;
    return (maxEnd + UBO_ALIGNMENT - 1) & ~(UBO_ALIGNMENT - 1);
}

void ParameterLayout::separateParameters(const SmallVector<MaterialParameterDesc>& parameters,
                                         SmallVector<MaterialParameterDesc>& uniformParams,
                                         SmallVector<MaterialParameterDesc>& textureParams)
{
    uniformParams.clear();
    textureParams.clear();

    for (const auto& param : parameters)
    {
        if (param.isTexture || TypeUtils::isTextureType(param.type))
        {
            textureParams.push_back(param);
        }
        else
        {
            uniformParams.push_back(param);
        }
    }
}
} // namespace aph
