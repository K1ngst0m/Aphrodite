#pragma once

#include "common/arrayProxy.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include "descriptorSet.h"
#include "vkUtils.h"

namespace aph::vk
{
class Buffer;
class Image;
class Sampler;
class Device;
class ShaderProgram;

class DataBuilder
{
public:
    explicit DataBuilder(uint32_t minAlignment) noexcept
        : m_minAlignment(minAlignment)
    {
        APH_ASSERT(minAlignment != 0 && (minAlignment & (minAlignment - 1)) == 0 &&
                   "minAlignment must be a power of 2!");
    }

    template <typename T_Data>
        requires std::is_trivially_copyable_v<T_Data> && (!std::is_pointer_v<T_Data>)
    void writeTo(T_Data& writePtr) noexcept
    {
        writeTo(&writePtr);
    }

    void writeTo(const void* writePtr) noexcept
    {
        std::memcpy((uint8_t*)writePtr, getData().data(), getData().size());
    }

    const std::vector<std::byte>& getData() const noexcept
    {
        return m_data;
    }
    std::vector<std::byte>& getData() noexcept
    {
        return m_data;
    }

    void reset() noexcept
    {
        m_data.clear();
    }

    template <typename T_Data>
    uint32_t addRange(T_Data dataRange, Range range = {})
    {
        if (range.size == 0)
        {
            range.size = sizeof(T_Data);
        }

        static_assert(std::is_trivially_copyable_v<T_Data>, "The range data must be trivially copyable");
        const std::byte* rawDataPtr = reinterpret_cast<const std::byte*>(&dataRange);

        APH_ASSERT(range.offset + range.size <= sizeof(T_Data));

        size_t bytesToCopy = range.size;

        uint32_t offset = aph::utils::paddingSize(m_minAlignment, m_data.size());

        size_t newSize = offset + bytesToCopy;
        if (newSize > m_data.size())
        {
            m_data.resize(newSize);
        }

        std::memcpy(m_data.data() + offset, rawDataPtr + range.offset, bytesToCopy);

        return offset;
    }

private:
    std::vector<std::byte> m_data;
    uint32_t m_minAlignment;
};

class BindlessResource
{
    enum ResourceType : uint32_t
    {
        eSampledImage = 0,
        eStorageImage = 1,
        eUniformBuffer = 2,
        eStorageBuffer = 3,
        eSampler = 4,
        ResourceTypeCount
    };

public:
    enum SetIdx
    {
        ResourceSetIdx = 0,
        HandleSetIdx = 1,
        UpperBound
    };

    struct HandleId
    {
        uint32_t id = std::numeric_limits<uint32_t>::max();
        operator uint32_t() const
        {
            return id;
        }
        static constexpr uint32_t InvalidId = std::numeric_limits<uint32_t>::max();
    };

    BindlessResource(ShaderProgram* pProgram, Device* pDevice);
    ~BindlessResource();

    void clear();

    template <typename T_Data>
    uint32_t addRange(T_Data&& dataRange, Range range = {})
    {
        auto offset = m_handleData.dataBuilder.addRange(std::forward<T_Data>(dataRange), range);

        // TODO dirty range
        m_rangeDirty = true;

        return offset;
    }

    void build();

    HandleId updateResource(Buffer* pBuffer, ::vk::BufferUsageFlagBits2 usage);
    HandleId updateResource(Image* pImage, ::vk::ImageUsageFlagBits usage);
    HandleId updateResource(Sampler* pSampler);

    DescriptorSetLayout* getResourceLayout() const noexcept
    {
        return m_resourceData.pSetLayout;
    }

    DescriptorSetLayout* getHandleLayout() const noexcept
    {
        return m_handleData.pSetLayout;
    }

    DescriptorSet* getResourceSet() const noexcept
    {
        APH_ASSERT(m_resourceData.pSet);
        return m_resourceData.pSet;
    }

    DescriptorSet* getHandleSet() const noexcept
    {
        APH_ASSERT(m_handleData.pSet);
        return m_handleData.pSet;
    }

private:
    Device* m_pDevice;

    struct Handle
    {
        Handle(uint32_t minAlignment)
            : dataBuilder(minAlignment)
        {
        }

        DataBuilder dataBuilder;
        Buffer* pBuffer = {};
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet = {};
    } m_handleData;

    struct Resource
    {
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet = {};
    } m_resourceData;

    bool m_rangeDirty = false;

    SmallVector<Image*> m_images;
    SmallVector<Buffer*> m_buffers;
    SmallVector<Sampler*> m_samplers;
    HashMap<Image*, HandleId> m_imageIds;
    HashMap<Buffer*, HandleId> m_bufferIds;
    HashMap<Sampler*, HandleId> m_samplerIds;

    SmallVector<DescriptorUpdateInfo> m_resourceUpdateInfos;
    std::mutex m_mtx;
};

} // namespace aph::vk
