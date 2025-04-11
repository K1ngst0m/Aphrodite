#include "bufferAsset.h"

namespace aph
{
//-----------------------------------------------------------------------------
// BufferAsset Implementation
//-----------------------------------------------------------------------------

BufferAsset::BufferAsset()
    : m_pBufferResource(nullptr)
    , m_pDevice(nullptr)
    , m_bufferUsage(BufferUsage::None)
    , m_contentType(BufferContentType::Unknown)
    , m_loadTimestamp(0)
    , m_isMapped(false)
{
}

BufferAsset::~BufferAsset()
{
    // The Resource loader is responsible for freeing the buffer resource
}

size_t BufferAsset::getSize() const
{
    if (m_pBufferResource)
    {
        return m_pBufferResource->getSize();
    }
    return 0;
}

BufferUsageFlags BufferAsset::getUsage() const
{
    if (m_pBufferResource)
    {
        return m_pBufferResource->getUsage();
    }
    return BufferUsage::None;
}

bool BufferAsset::isMapped() const
{
    return m_isMapped;
}

vk::Buffer* BufferAsset::getBuffer() const
{
    return m_pBufferResource;
}

void* BufferAsset::map(size_t offset, size_t size)
{
    if (!m_pBufferResource || !m_pDevice)
    {
        return nullptr;
    }

    // Call mapMemory with just the buffer pointer - it doesn't accept offset/size
    void* mappedData = m_pDevice->mapMemory(m_pBufferResource);

    if (mappedData)
    {
        // If we have a valid offset, adjust the pointer accordingly
        // (the API maps the whole buffer, so we adjust the pointer ourselves)
        if (offset > 0 && offset < getSize())
        {
            mappedData = static_cast<uint8_t*>(mappedData) + offset;
        }

        m_isMapped = true;
    }

    return mappedData;
}

void BufferAsset::unmap()
{
    if (!m_pBufferResource || !m_isMapped || !m_pDevice)
    {
        return;
    }

    m_pDevice->unMapMemory(m_pBufferResource);
    m_isMapped = false;
}

Result BufferAsset::update(const BufferUpdateInfo& updateInfo)
{
    if (!m_pBufferResource)
    {
        return {Result::RuntimeError, "Buffer not initialized"};
    }

    void* mappedData = map(updateInfo.range.offset, updateInfo.range.size);
    if (!mappedData)
    {
        return {Result::RuntimeError, "Failed to map buffer for update"};
    }

    size_t copySize =
        (updateInfo.range.size == VK_WHOLE_SIZE) ? getSize() - updateInfo.range.offset : updateInfo.range.size;

    std::memcpy(mappedData, updateInfo.data, copySize);
    unmap();

    return Result::Success;
}

void BufferAsset::setBufferResource(vk::Buffer* pBuffer, BufferUsageFlags usage)
{
    m_pBufferResource = pBuffer;
    m_bufferUsage     = usage;
}

void BufferAsset::setLoadInfo(const std::string& sourceDesc, const std::string& debugName,
                              BufferContentType contentType)
{
    m_sourceDesc    = sourceDesc;
    m_debugName     = debugName;
    m_contentType   = contentType;
    m_loadTimestamp = std::chrono::steady_clock::now().time_since_epoch().count();
}

void BufferAsset::setDevice(vk::Device* pDevice)
{
    m_pDevice = pDevice;
}

std::string BufferAsset::getUsageString() const
{
    std::stringstream ss;
    auto usage = getUsage();

    if (usage == BufferUsage::None)
    {
        return "None";
    }

    if (usage & BufferUsage::Vertex)
        ss << "Vertex ";
    if (usage & BufferUsage::Index)
        ss << "Index ";
    if (usage & BufferUsage::Uniform)
        ss << "Uniform ";
    if (usage & BufferUsage::Storage)
        ss << "Storage ";
    if (usage & BufferUsage::Indirect)
        ss << "Indirect ";
    if (usage & BufferUsage::TransferSrc)
        ss << "TransferSrc ";
    if (usage & BufferUsage::TransferDst)
        ss << "TransferDst ";

    return ss.str();
}

std::string BufferAsset::getContentTypeString() const
{
    switch (m_contentType)
    {
    case BufferContentType::Vertex:
        return "Vertex Data";
    case BufferContentType::Index:
        return "Index Data";
    case BufferContentType::Uniform:
        return "Uniform Data";
    case BufferContentType::Storage:
        return "Storage Data";
    case BufferContentType::Indirect:
        return "Indirect Commands";
    case BufferContentType::RawData:
        return "Raw Data";
    default:
        return "Unknown";
    }
}

std::string BufferAsset::getInfoString() const
{
    std::stringstream ss;

    // Basic buffer properties
    ss << "Buffer: " << (m_debugName.empty() ? "Unnamed" : m_debugName) << "\n";
    ss << "Size: " << getSize() << " bytes\n";

    // Usage and content type
    ss << "Usage: " << getUsageString() << "\n";
    ss << "Content Type: " << getContentTypeString() << "\n";

    // Source info
    ss << "Source: " << (m_sourceDesc.empty() ? "Unknown" : m_sourceDesc);

    return ss.str();
}

} // namespace aph
