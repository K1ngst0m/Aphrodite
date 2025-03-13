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
        eImage = 0,
        eBuffer = 1,
        eSampler = 2,
        eResourceTypeCount
    };

public:
    enum SetIdx
    {
        eResourceSetIdx = 0,
        eHandleSetIdx = 1,
        eUpperBound
    };

    struct HandleId
    {
        uint32_t id = InvalidId;
        operator uint32_t() const
        {
            return id;
        }
        static constexpr uint32_t InvalidId = std::numeric_limits<uint32_t>::max();
    };

    BindlessResource(Device* pDevice);
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

    HandleId updateResource(Buffer* pBuffer);
    HandleId updateResource(Image* pImage);
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

    ::vk::PipelineLayout getPipelineLayout() const noexcept
    {
        return m_pipelineLayout.handle;
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
        static constexpr std::size_t AddressTableSize = 4 * memory::KB;
        Buffer* pAddressTableBuffer = {};
        std::span<uint64_t> addressTableMap;
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

    PipelineLayout m_pipelineLayout{};
};

} // namespace aph::vk
