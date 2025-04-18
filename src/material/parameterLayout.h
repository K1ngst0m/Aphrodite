#pragma once

#include "materialTemplate.h"
#include "typeUtils.h"

namespace aph
{
// Helper class to generate properly aligned parameter layouts from a template
class ParameterLayout
{
public:
    // Calculate aligned offset for a parameter
    [[nodiscard]] static auto calculateAlignedOffset(uint32_t currentOffset, DataType type) -> uint32_t;

    // Calculate total size needed for parameters
    [[nodiscard]] static auto calculateTotalSize(const SmallVector<MaterialParameterDesc>& parameters) -> uint32_t;

    // Generate a parameter layout from a material template
    [[nodiscard]] static auto generateLayout(const MaterialTemplate* pTemplate) -> SmallVector<MaterialParameterDesc>;

    // Separate parameters by type (uniform vs texture)
    static void separateParameters(const SmallVector<MaterialParameterDesc>& parameters,
                                   SmallVector<MaterialParameterDesc>& uniformParams,
                                   SmallVector<MaterialParameterDesc>& textureParams);
};

} // namespace aph