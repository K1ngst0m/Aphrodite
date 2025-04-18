#include "materialAsset.h"
#include "common/logger.h"

namespace aph
{

MaterialAsset::MaterialAsset(Material* pMaterial)
    : m_pMaterial(pMaterial)
{
}

auto MaterialAsset::isLoaded() const -> bool
{
    return m_pMaterial != nullptr;
}

auto MaterialAsset::getMaterial() const -> Material*
{
    return m_pMaterial;
}

auto MaterialAsset::getPath() const -> std::string_view
{
    return m_path;
}

auto MaterialAsset::isModified() const -> bool
{
    return m_isModified;
}

auto MaterialAsset::getTimestamp() const -> uint64_t
{
    return m_timestamp;
}

} // namespace aph
