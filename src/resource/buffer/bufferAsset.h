#pragma once

#include "api/vulkan/device.h"

namespace aph
{
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
    std::string debugName           = {};
    const void* data                = {};
    size_t dataSize                 = 0;
    vk::BufferCreateInfo createInfo = {};
    BufferContentType contentType   = BufferContentType::RawData;
    bool forceUncached              = false;
};

struct BufferUpdateInfo
{
    const void* data = {};
    Range range      = {0, VK_WHOLE_SIZE};
};
class BufferAsset
{
public:
    BufferAsset();
    ~BufferAsset();

    // Accessors - delegate to vk::Buffer for buffer properties
    auto getSize() const -> size_t;
    auto getUsage() const -> BufferUsageFlags;

    // Mid-level loading info accessors
    auto getSourceDesc() const -> const std::string&;
    auto getDebugName() const -> const std::string&;
    auto getContentType() const -> BufferContentType;
    auto isValid() const -> bool;
    auto getLoadTimestamp() const -> uint64_t;
    auto isMapped() const -> bool;

    // Utility methods
    auto getInfoString() const -> std::string;
    auto getUsageString() const -> std::string;
    auto getContentTypeString() const -> std::string;

    // Resource access
    auto getBuffer() const -> vk::Buffer*;

    // Data mapping methods
    auto map(size_t offset = 0, size_t size = VK_WHOLE_SIZE) -> void*;
    void unmap();

    // Update buffer data
    auto update(const BufferUpdateInfo& updateInfo) -> Result;

    // Internal use by the buffer loader
    void setBufferResource(vk::Buffer* pBuffer, BufferUsageFlags usage);
    void setLoadInfo(const std::string& sourceDesc, const std::string& debugName, BufferContentType contentType);
    void setDevice(vk::Device* pDevice);

private:
    vk::Buffer* m_pBufferResource;
    vk::Device* m_pDevice;
    BufferUsageFlags m_bufferUsage;

    std::string m_sourceDesc; // Description of source (raw data, file, etc.)
    std::string m_debugName; // Debug name used for the resource
    BufferContentType m_contentType;
    uint64_t m_loadTimestamp; // When the buffer was loaded
    bool m_isMapped; // Whether the buffer is currently mapped
};

} // namespace aph
