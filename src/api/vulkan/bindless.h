#pragma once

#include "common/arrayProxy.h"
#include "common/dataBuilder.h"
#include "common/hash.h"
#include "common/profiler.h"
#include "common/smallVector.h"
#include "descriptorSet.h"
#include "forward.h"
#include "vkUtils.h"
#include <bitset>

namespace aph::vk
{
class BindlessResource
{
public:
    // Core types and enums
    enum SetIdx
    {
        eResourceSetIdx = 0, // Index of resource descriptor set (textures, buffers, samplers)
        eHandleSetIdx   = 1, // Index of handle descriptor set (resource indices)
        eUpperBound
    };

    struct HandleId
    {
        uint32_t id = kInvalidId;

        operator uint32_t() const
        {
            return id;
        }

        static constexpr uint32_t kInvalidId = std::numeric_limits<uint32_t>::max();
    };

    // Variant type that can hold any supported resource type
    using RType = std::variant<Image*, Buffer*, Sampler*>;

    // Constructor and destructor
    explicit BindlessResource(Device* pDevice);
    ~BindlessResource();

    // Resource management
    auto updateResource(RType resource, std::string name) -> uint32_t;
    auto updateResource(Buffer* pBuffer) -> HandleId;
    auto updateResource(Image* pImage) -> HandleId;
    auto updateResource(Sampler* pSampler) -> HandleId;

    // Handle buffer operations
    template <typename T_Data>
    auto addRange(T_Data&& dataRange, Range range = {}) -> uint32_t;
    auto build() -> void;
    auto clear() -> void;

    // Shader code generation
    auto generateHandleSource() const -> std::string;

    // Accessors
    auto getResourceLayout() const noexcept -> DescriptorSetLayout*;
    auto getHandleLayout() const noexcept -> DescriptorSetLayout*;
    auto getResourceSet() const noexcept -> DescriptorSet*;
    auto getHandleSet() const noexcept -> DescriptorSet*;
    auto getPipelineLayout() const noexcept -> PipelineLayout*;

private:
    // Internal types
    enum ResourceType : uint8_t
    {
        eImage   = 0,
        eBuffer  = 1,
        eSampler = 2,
        eResourceTypeCount
    };

    // Handle data storage and management
    struct Handle
    {
        Handle(uint32_t minAlignment)
            : dataBuilder(minAlignment)
        {
        }

        DataBuilder dataBuilder; // Builder for CPU-side handle data
        Buffer* pBuffer                 = {}; // GPU buffer containing handle data
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet             = {};
    } m_handleData;

    // Resource data storage and management
    struct Resource
    {
        static constexpr std::size_t AddressTableSize = 4 * memory::KB;
        Buffer* pAddressTableBuffer                   = {}; // GPU buffer for buffer addresses
        std::span<uint64_t> addressTableMap; // Mapped view of address table
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet             = {};
    } m_resourceData;

    // Member variables
    Device* m_pDevice                = {};
    PipelineLayout* m_pipelineLayout = {};
    std::atomic<bool> m_rangeDirty{ false };

    // Resource collections
    SmallVector<Image*> m_images;
    SmallVector<Buffer*> m_buffers;
    SmallVector<Sampler*> m_samplers;
    HashMap<Image*, HandleId> m_imageIds;
    HashMap<Buffer*, HandleId> m_bufferIds;
    HashMap<Sampler*, HandleId> m_samplerIds;
    HashMap<std::string, RType> m_handleNameMap;
    SmallVector<DescriptorUpdateInfo> m_resourceUpdateInfos;

    // Synchronization
    mutable std::mutex m_handleMtx;
    mutable std::shared_mutex m_nameMtx;
    mutable std::shared_mutex m_resourceMapsMtx;
    mutable std::mutex m_updateInfoMtx;
};

template <typename TData>
inline auto BindlessResource::addRange(TData&& dataRange, Range range) -> uint32_t
{
    std::lock_guard<std::mutex> lock{ m_handleMtx };
    auto offset = m_handleData.dataBuilder.addRange(std::forward<TData>(dataRange), range);
    m_rangeDirty.store(true, std::memory_order_release);
    return offset;
}

} // namespace aph::vk
