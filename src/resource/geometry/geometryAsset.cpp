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

uint32_t GeometryAsset::getSubmeshCount() const
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getSubmeshCount();
    }
    return 0;
}

const Submesh* GeometryAsset::getSubmesh(uint32_t index) const
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getSubmesh(index);
    }
    return nullptr;
}

BoundingBox GeometryAsset::getBoundingBox() const
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getBoundingBox();
    }
    return {};
}

bool GeometryAsset::supportsMeshShading() const
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->supportsMeshShading();
    }
    return false;
}

void GeometryAsset::bind(vk::CommandBuffer* cmdBuffer)
{
    APH_PROFILER_SCOPE();

    if (m_pGeometryResource)
    {
        m_pGeometryResource->bind(cmdBuffer);
    }
}

void GeometryAsset::draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex, uint32_t instanceCount)
{
    APH_PROFILER_SCOPE();

    if (m_pGeometryResource)
    {
        m_pGeometryResource->draw(cmdBuffer, submeshIndex, instanceCount);
    }
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

uint32_t GeometryAsset::getMaterialIndex(uint32_t submeshIndex) const
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

} // namespace aph
