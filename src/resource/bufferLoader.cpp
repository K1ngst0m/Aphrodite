#include "bufferLoader.h"

#include "common/common.h"
#include "common/profiler.h"
#include "resourceLoader.h"

namespace aph
{
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
