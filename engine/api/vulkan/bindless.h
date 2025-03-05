#pragma once

#include "common/hash.h"
#include "common/smallVector.h"
#include "vkUtils.h"

namespace aph::vk
{
class Buffer;
class Image;
class Sampler;
class Device;
class DescriptorSet;
class DescriptorSetLayout;
class ShaderProgram;

class DataBuilder
{
public:
    explicit DataBuilder(uint32_t minAlignment)
        : m_minAlignment(minAlignment)
    {
        APH_ASSERT((minAlignment & (minAlignment - 1)) == 0 && "minAlignment must be a power of 2!");
    }

    template <typename T_Data>
        requires std::is_trivially_copyable_v<T_Data> && (!std::is_pointer_v<T_Data>)
    void writeTo(T_Data& writePtr)
    {
        writeTo(&writePtr);
    }

    void writeTo(const void* writePtr)
    {
        std::memcpy((uint8_t*)writePtr, getData().data(), getData().size());
    }

    const std::vector<std::byte>& getData() const
    {
        return m_data;
    }
    std::vector<std::byte>& getData()
    {
        return m_data;
    }

    template <typename T_Data>
    uint32_t addRange(T_Data&& dataRange)
    {
        static_assert(std::is_trivially_copyable_v<T_Data>, "The range data must be trivially copyable");
        const std::byte* rawDataPtr = reinterpret_cast<const std::byte*>(&dataRange);

        size_t dataBytes = sizeof(dataRange);

        uint32_t offset = aph::utils::paddingSize(static_cast<uint32_t>(m_data.size()), m_minAlignment);

        size_t newSize = offset + dataBytes;
        if (newSize > m_data.size())
        {
            m_data.resize(newSize);
        }

        std::memcpy(m_data.data() + offset, rawDataPtr, dataBytes);

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
    struct HandleId
    {
        explicit HandleId(uint32_t id = HandleId::InvalidId)
            : id(id)
        {
        }
        operator uint32_t() const
        {
            return id;
        }
        uint32_t id;
        static constexpr uint32_t InvalidId = std::numeric_limits<uint32_t>::max();
    };

    BindlessResource(DescriptorSetLayout* pSetLayout, Device* pDevice);

    template <typename T_Data>
    uint32_t addRange(T_Data&& dataRange)
    {
        auto offset = m_handleResource.dataBuilder.addRange(std::forward<T_Data>(dataRange));

        // TODO dirty range

        return offset;
    }

    void buildHandleBuffer(DescriptorSetLayout* pSetLayout);

    HandleId updateResource(Buffer* pBuffer, ::vk::BufferUsageFlagBits2 usage);
    HandleId updateResource(Image* pImage, ::vk::ImageUsageFlagBits usage);
    HandleId updateResource(Sampler* pSampler);

    DescriptorSet* getResourceSet() const
    {
        APH_ASSERT(m_pSet);
        return m_pSet;
    }

    DescriptorSet* getHandleSet() const
    {
        APH_ASSERT(m_handleResource.pSet);
        return m_handleResource.pSet;
    }

private:
    Device* m_pDevice;

    struct HandleResource
    {
        HandleResource(uint32_t minAlignment)
            : dataBuilder(minAlignment)
        {
        }

        DataBuilder dataBuilder;
        Buffer* pBuffer = {};
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet = {};
    } m_handleResource;

    SmallVector<Image*> m_images;
    SmallVector<Buffer*> m_buffers;
    SmallVector<Sampler*> m_samplers;
    HashMap<Image*, HandleId> m_imageIds;
    HashMap<Buffer*, HandleId> m_bufferIds;
    HashMap<Sampler*, HandleId> m_samplerIds;

    DescriptorSetLayout* m_pSetLayout = {};
    DescriptorSet* m_pSet = {};
};

} // namespace aph::vk
