#pragma once

#include "api/vulkan/device.h"

namespace aph
{
// Forward declarations
class ResourceLoader;

enum class BufferContentType
{
    Unknown,
    Vertex,
    Index,
    Uniform,
    Storage,
    Indirect,
    RawData
};

struct BufferLoadInfo
{
    std::string debugName = {};
    const void* data = {};
    size_t dataSize = 0;
    vk::BufferCreateInfo createInfo = {};
    BufferContentType contentType = BufferContentType::RawData;
};

struct BufferUpdateInfo
{
    const void* data = {};
    Range range = { 0, VK_WHOLE_SIZE };
};

class BufferAsset
{
public:
    BufferAsset();
    ~BufferAsset();

    // Accessors - delegate to vk::Buffer for buffer properties
    size_t getSize() const;
    BufferUsageFlags getUsage() const;
    
    // Mid-level loading info accessors
    const std::string& getSourceDesc() const { return m_sourceDesc; }
    const std::string& getDebugName() const { return m_debugName; }
    BufferContentType getContentType() const { return m_contentType; }
    bool isValid() const { return m_pBufferResource != nullptr; }
    uint64_t getLoadTimestamp() const { return m_loadTimestamp; }
    bool isMapped() const;
    
    // Utility methods
    std::string getInfoString() const;
    std::string getUsageString() const;
    std::string getContentTypeString() const;

    // Resource access
    vk::Buffer* getBuffer() const;
    
    // Data mapping methods
    void* map(size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    void unmap();
    
    // Update buffer data
    Result update(const BufferUpdateInfo& updateInfo);

    // Internal use by the buffer loader
    void setBufferResource(vk::Buffer* pBuffer, BufferUsageFlags usage);
    void setLoadInfo(const std::string& sourceDesc, const std::string& debugName, 
                    BufferContentType contentType);
    void setDevice(vk::Device* pDevice);

private:
    vk::Buffer* m_pBufferResource;
    vk::Device* m_pDevice;
    BufferUsageFlags m_bufferUsage;
    
    std::string m_sourceDesc;    // Description of source (raw data, file, etc.)
    std::string m_debugName;     // Debug name used for the resource
    BufferContentType m_contentType;
    uint64_t m_loadTimestamp;    // When the buffer was loaded
    bool m_isMapped;             // Whether the buffer is currently mapped
};

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
