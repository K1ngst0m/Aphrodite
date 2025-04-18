#pragma once

#include "common/smallVector.h"
#include "materialTemplate.h"

namespace aph
{
// Class to manage parameter layouts with proper alignment
class ParameterLayout
{
public:
    ParameterLayout() = default;
    ~ParameterLayout() = default;

    void addParameter(const MaterialParameterDesc& param);
    [[nodiscard]] auto getParameters() const -> const SmallVector<MaterialParameterDesc>&;
    [[nodiscard]] auto getAlignedLayout() const -> const SmallVector<MaterialParameterDesc>&;
    [[nodiscard]] auto getTotalSize() const -> uint32_t;
private:
    void markDirty();
    void generateAlignedLayout() const;

    [[nodiscard]] static auto calculateAlignedOffset(uint32_t currentOffset, DataType type) -> uint32_t;
    [[nodiscard]] auto separateParameters(const SmallVector<MaterialParameterDesc>& parameters) const 
        -> std::tuple<SmallVector<MaterialParameterDesc>, SmallVector<MaterialParameterDesc>>;

private:
    SmallVector<MaterialParameterDesc> m_parameters;
    mutable SmallVector<MaterialParameterDesc> m_alignedLayout;
    mutable uint32_t m_totalSize{0};
    mutable bool m_isDirty{true};
};

} // namespace aph
