#pragma once

#include "api/vulkan/device.h"
#include "bufferAsset.h"
#include "resource/forward.h"

namespace aph
{
// Forward declarations
class ResourceLoader;

// Buffer loader class (internal to the resource system)
class BufferLoader
{
public:
    BufferLoader(ResourceLoader* pResourceLoader);
    ~BufferLoader();

    // Load a buffer asset from raw data
    Result loadFromData(const BufferLoadInfo& info, BufferAsset** ppBufferAsset);

    // Update existing buffer with new data
    Result updateBuffer(BufferAsset* pBufferAsset, const BufferUpdateInfo& updateInfo);

    // Destroy a buffer asset
    void destroy(BufferAsset* pBufferAsset);

private:
    // Create GPU resources for the buffer
    Result createBufferResources(const BufferLoadInfo& info, BufferAsset** ppBufferAsset);

private:
    ResourceLoader* m_pResourceLoader = {};
    ThreadSafeObjectPool<BufferAsset> m_bufferAssetPools;
};
} // namespace aph
