#pragma once

#include "api/vulkan/device.h"
#include "bufferAsset.h"
#include "resource/forward.h"

namespace aph
{
class ResourceLoader;

class BufferLoader
{
public:
    explicit BufferLoader(ResourceLoader* pResourceLoader);
    ~BufferLoader();

    auto load(const BufferLoadInfo& info, BufferAsset** ppBufferAsset) -> Result;
    auto update(BufferAsset* pBufferAsset, const BufferUpdateInfo& updateInfo) -> Result;
    void unload(BufferAsset* pBufferAsset);

private:
    auto createBufferResources(const BufferLoadInfo& info, BufferAsset** ppBufferAsset) -> Result;

private:
    ResourceLoader* m_pResourceLoader = {};
    ThreadSafeObjectPool<BufferAsset> m_bufferAssetPools;
};
} // namespace aph
