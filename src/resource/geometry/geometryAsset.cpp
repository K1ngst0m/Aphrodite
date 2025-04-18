#include "geometryAsset.h"

#include "common/profiler.h"

namespace aph
{

GeometryAsset::GeometryAsset()
    : m_pGeometryResource(nullptr)
{
}

GeometryAsset::~GeometryAsset() = default;

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

// Buffer accessors implementation
auto GeometryAsset::getPositionBuffer() const -> vk::Buffer*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getPositionBuffer();
    }
    return nullptr;
}

auto GeometryAsset::getAttributeBuffer() const -> vk::Buffer*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getAttributeBuffer();
    }
    return nullptr;
}

auto GeometryAsset::getIndexBuffer() const -> vk::Buffer*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getIndexBuffer();
    }
    return nullptr;
}

auto GeometryAsset::getMeshletBuffer() const -> vk::Buffer*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getMeshletBuffer();
    }
    return nullptr;
}

auto GeometryAsset::getMeshletVertexBuffer() const -> vk::Buffer*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getMeshletVertexBuffer();
    }
    return nullptr;
}

auto GeometryAsset::getMeshletIndexBuffer() const -> vk::Buffer*
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getMeshletIndexBuffer();
    }
    return nullptr;
}

// Statistics accessors implementation
auto GeometryAsset::getVertexCount() const -> uint32_t
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getVertexCount();
    }
    return 0;
}

auto GeometryAsset::getIndexCount() const -> uint32_t
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getIndexCount();
    }
    return 0;
}

auto GeometryAsset::getMeshletCount() const -> uint32_t
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getMeshletCount();
    }
    return 0;
}

auto GeometryAsset::getMeshletMaxVertexCount() const -> uint32_t
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getMeshletMaxVertexCount();
    }
    return 0;
}

auto GeometryAsset::getMeshletMaxTriangleCount() const -> uint32_t
{
    if (m_pGeometryResource)
    {
        return m_pGeometryResource->getMeshletMaxTriangleCount();
    }
    return 0;
}

auto GeometryAsset::submeshes() const -> coro::generator<const Submesh*>
{
    for (uint32_t i = 0; i < getSubmeshCount(); i++)
    {
        const Submesh* submesh = getSubmesh(i);
        co_yield submesh;
    }
}
} // namespace aph
