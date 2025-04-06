#include "bufferLoader.h"

#include "common/common.h"
#include "common/profiler.h"
#include "resourceLoader.h"

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
    m_bufferUsage = usage;
}

void BufferAsset::setLoadInfo(const std::string& sourceDesc, const std::string& debugName,
                              BufferContentType contentType)
{
    m_sourceDesc = sourceDesc;
    m_debugName = debugName;
    m_contentType = contentType;
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

//-----------------------------------------------------------------------------
// BufferLoader Implementation
//-----------------------------------------------------------------------------

BufferLoader::BufferLoader(ResourceLoader* pResourceLoader)
    : m_pResourceLoader(pResourceLoader)
{
}

BufferLoader::~BufferLoader()
{
}

Result BufferLoader::loadFromData(const BufferLoadInfo& info, BufferAsset** ppBufferAsset)
{
    APH_PROFILER_SCOPE();

    // Validate inputs
    if (!info.data && info.dataSize > 0)
    {
        return {Result::RuntimeError, "Buffer data is null but size is non-zero"};
    }

    // Check the create info
    if (info.createInfo.size == 0)
    {
        return {Result::RuntimeError, "Buffer size cannot be zero"};
    }

    // If dataSize is provided, ensure it doesn't exceed buffer size
    if (info.dataSize > info.createInfo.size)
    {
        return {Result::RuntimeError, "Data size exceeds buffer size"};
    }

    // Create buffer resources
    return createBufferResources(info, ppBufferAsset);
}

Result BufferLoader::updateBuffer(BufferAsset* pBufferAsset, const BufferUpdateInfo& updateInfo)
{
    APH_PROFILER_SCOPE();

    if (!pBufferAsset)
    {
        return {Result::RuntimeError, "Buffer asset is null"};
    }

    return pBufferAsset->update(updateInfo);
}

void BufferLoader::destroy(BufferAsset* pBufferAsset)
{
    if (pBufferAsset)
    {
        // Ensure buffer is unmapped before destruction
        if (pBufferAsset->isMapped())
        {
            pBufferAsset->unmap();
        }

        // Destroy the underlying buffer resource
        vk::Buffer* pBuffer = pBufferAsset->getBuffer();
        if (pBuffer)
        {
            m_pResourceLoader->getDevice()->destroy(pBuffer);
        }

        // Delete the asset
        m_bufferAssetPools.free(pBufferAsset);
    }
}

Result BufferLoader::createBufferResources(const BufferLoadInfo& info, BufferAsset** ppBufferAsset)
{
    APH_PROFILER_SCOPE();

    // Create the asset
    *ppBufferAsset = m_bufferAssetPools.allocate();

    // Get the device
    vk::Device* pDevice = m_pResourceLoader->getDevice();

    // Store the device in the asset for later use
    (*ppBufferAsset)->setDevice(pDevice);

    // Create buffer
    vk::Buffer* buffer;
    {
        auto bufferCI = info.createInfo;

        // Ensure we have transfer destination usage if we're initializing with data
        if (info.data && info.dataSize > 0)
        {
            bufferCI.usage |= BufferUsage::TransferDst;
        }

        // Set memory domain based on the usage
        if (bufferCI.domain == MemoryDomain::Auto)
        {
            // If the buffer is used as a uniform or storage buffer that needs CPU access,
            // use host memory, otherwise use device memory
            if (bufferCI.usage & (BufferUsage::Uniform | BufferUsage::Storage))
            {
                bufferCI.domain = MemoryDomain::Host;
            }
            else
            {
                bufferCI.domain = MemoryDomain::Device;
            }
        }

        auto bufferResult = pDevice->create(bufferCI, info.debugName);
        APH_VERIFY_RESULT(bufferResult);
        buffer = bufferResult.value();
    }

    // If we have data to initialize the buffer with
    if (info.data && info.dataSize > 0)
    {
        // For Host or Upload memory, we can map directly
        // For Device memory, we need a staging buffer
        if (info.createInfo.domain == MemoryDomain::Host || info.createInfo.domain == MemoryDomain::Upload)
        {
            void* pMapped = pDevice->mapMemory(buffer);
            if (pMapped)
            {
                std::memcpy(pMapped, info.data, info.dataSize);
                pDevice->unMapMemory(buffer);
            }
        }
        else
        {
            // Otherwise, we need to use a staging buffer
            vk::Buffer* stagingBuffer;
            vk::BufferCreateInfo stagingCI{
                .size = info.dataSize,
                .usage = BufferUsage::TransferSrc,
                .domain = MemoryDomain::Upload,
            };

            auto stagingResult = pDevice->create(stagingCI, std::string{info.debugName} + std::string{"_staging"});
            APH_VERIFY_RESULT(stagingResult);
            stagingBuffer = stagingResult.value();

            // Map and copy data to staging buffer
            void* pMapped = pDevice->mapMemory(stagingBuffer);
            APH_ASSERT(pMapped);
            std::memcpy(pMapped, info.data, info.dataSize);
            pDevice->unMapMemory(stagingBuffer);

            // Copy from staging buffer to destination buffer
            vk::Queue* pTransferQueue = pDevice->getQueue(QueueType::Transfer);
            pDevice->executeCommand(pTransferQueue, [&](auto* cmd)
                                    { cmd->copy(stagingBuffer, buffer, Range{.offset = 0, .size = info.dataSize}); });

            // Cleanup staging buffer
            pDevice->destroy(stagingBuffer);
        }
    }

    // Set the buffer and its usage in the asset
    (*ppBufferAsset)->setBufferResource(buffer, info.createInfo.usage);

    // Set loading information
    std::string sourceDesc = "Raw data buffer (" + std::to_string(info.dataSize) + " bytes)";
    (*ppBufferAsset)->setLoadInfo(sourceDesc, info.debugName, info.contentType);

    return Result::Success;
}

} // namespace aph
