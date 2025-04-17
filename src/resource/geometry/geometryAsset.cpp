#include "geometryAsset.h"

#include "common/profiler.h"

namespace aph
{

GeometryAsset::GeometryAsset()
    : m_pGeometryResource(nullptr)
{
}

GeometryAsset::~GeometryAsset()
{
    // The unique_ptr will clean up the resource
}

auto GeometryAsset::getSubmeshCount() const -> uint32_t
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getSubmeshCount();
    }
    return 0;
}

auto GeometryAsset::getSubmesh(uint32_t index) const -> const Submesh*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getSubmesh(index);
    }
    return nullptr;
}

auto GeometryAsset::getBoundingBox() const -> BoundingBox
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getBoundingBox();
    }
    return {};
}

auto GeometryAsset::supportsMeshShading() const -> bool
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->supportsMeshShading();
    }
    return false;
}

void GeometryAsset::setMaterialIndex(uint32_t submeshIndex, uint32_t materialIndex)
{
    if (m_pGeometryResource && submeshIndex < getSubmeshCount())
    {
        // This would need to modify the submesh material index
        // For now, we just keep it read-only
        // In a full implementation, we'd need to handle this properly
    }
}

auto GeometryAsset::getMaterialIndex(uint32_t submeshIndex) const -> uint32_t
{
    if (m_pGeometryResource && submeshIndex < getSubmeshCount())
    {
        const Submesh* submesh = getSubmesh(submeshIndex);
        if (submesh != nullptr)
        {
            return submesh->materialIndex;
        }
    }
    return 0;
}

void GeometryAsset::setGeometryResource(std::unique_ptr<IGeometryResource> pResource)
{
    m_pGeometryResource = std::move(pResource);
}

auto GeometryAsset::getGeometryResource() const -> IGeometryResource*
{
    return m_pGeometryResource.get();
}
} // namespace aph
