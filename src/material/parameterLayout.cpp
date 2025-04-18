#include "parameterLayout.h"
#include "common/logger.h"
#include "materialTemplate.h"
#include "typeUtils.h"

namespace aph
{
void ParameterLayout::addParameter(const MaterialParameterDesc& param)
{
    m_parameters.push_back(param);
    markDirty();
}

auto ParameterLayout::getParameters() const -> const SmallVector<MaterialParameterDesc>&
{
    return m_parameters;
}

auto ParameterLayout::getAlignedLayout() const -> const SmallVector<MaterialParameterDesc>&
{
    if (m_isDirty)
    {
        generateAlignedLayout();
    }
    return m_alignedLayout;
}

auto ParameterLayout::getTotalSize() const -> uint32_t
{
    if (m_isDirty)
    {
        generateAlignedLayout();
    }
    return m_totalSize;
}

void ParameterLayout::markDirty()
{
    m_isDirty = true;
}

void ParameterLayout::generateAlignedLayout() const
{
    m_alignedLayout.clear();

    if (m_parameters.empty())
    {
        m_totalSize = 0;
        m_isDirty   = false;
        return;
    }

    // Separate parameters into uniform and texture params
    auto [uniformParams, textureParams] = separateParameters(m_parameters);

    // Process uniform parameters first (these go into the uniform buffer)
    uint32_t currentOffset = 0;

    for (const auto& param : uniformParams)
    {
        MaterialParameterDesc alignedParam = param;
        alignedParam.offset                = calculateAlignedOffset(currentOffset, param.type);
        currentOffset                      = alignedParam.offset + alignedParam.size;

        m_alignedLayout.push_back(alignedParam);
    }

    // Process texture parameters (these are bound as descriptors)
    currentOffset = 0;

    for (const auto& param : textureParams)
    {
        MaterialParameterDesc alignedParam = param;
        alignedParam.offset                = currentOffset;
        currentOffset += alignedParam.size;

        m_alignedLayout.push_back(alignedParam);
    }

    // Calculate total size with appropriate alignment
    uint32_t maxEnd = 0;
    for (const auto& param : m_alignedLayout)
    {
        if (!param.isTexture)
        {
            uint32_t end = param.offset + param.size;
            maxEnd       = std::max(maxEnd, end);
        }
    }

    // Ensure the total size is aligned to a proper boundary for UBO requirements
    constexpr uint32_t uboAlignment = 16;
    m_totalSize                     = (maxEnd + uboAlignment - 1) & ~(uboAlignment - 1);

    m_isDirty = false;
}

auto ParameterLayout::calculateAlignedOffset(uint32_t currentOffset, DataType type) -> uint32_t
{
    uint32_t alignment = TypeUtils::getTypeAlignment(type);
    return (currentOffset + alignment - 1) & ~(alignment - 1);
}

auto ParameterLayout::separateParameters(const SmallVector<MaterialParameterDesc>& parameters) const
    -> std::tuple<SmallVector<MaterialParameterDesc>, SmallVector<MaterialParameterDesc>>
{
    SmallVector<MaterialParameterDesc> uniformParams;
    SmallVector<MaterialParameterDesc> textureParams;

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

    return { std::move(uniformParams), std::move(textureParams) };
}
} // namespace aph
