#pragma once

#include "forward.h"
#include "vkUtils.h"
#include "common/arrayProxy.h"
#include "common/dataBuilder.h"
#include "common/hash.h"
#include "common/profiler.h"
#include "common/smallVector.h"
#include "descriptorSet.h"
#include <bitset>

namespace aph::vk
{
class BindlessResource
{
public:
    enum SetIdx
    {
        eResourceSetIdx = 0,   // Index of resource descriptor set (textures, buffers, samplers)
        eHandleSetIdx = 1,     // Index of handle descriptor set (resource indices)
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

    explicit BindlessResource(Device* pDevice);
    ~BindlessResource();

    // Variant type that can hold any supported resource type
    using RType = std::variant<Image*, Buffer*, Sampler*>;


    /**
     * @brief Registers a resource with the bindless system and gives it a name
     * 
     * This method registers a resource (image, buffer, or sampler) and associates
     * it with a name that can be used in shader code. The resource is assigned a
     * unique identifier, and descriptor updates are scheduled if needed.
     * 
     * Thread-safe: Uses multiple synchronization points to ensure consistency.
     * 
     * @param resource The resource to register (image, buffer, or sampler)
     * @param name The name to associate with this resource
     * @return The offset of the resource handle in the handle buffer
     */
    uint32_t updateResource(RType resource, std::string name);
    
    /**
     * @brief Adds raw data to the handle buffer
     * 
     * Allows adding arbitrary data to the handle buffer. Typically used to add
     * resource identifiers but can be used for other purposes.
     * 
     * Thread-safe: Protected by handle mutex.
     * 
     * @param dataRange The data to add
     * @param range Optional range within the data to add
     * @return The offset of the data in the handle buffer
     */
    template <typename T_Data>
    uint32_t addRange(T_Data&& dataRange, Range range = {})
    {
        std::lock_guard<std::mutex> lock{ m_handleMtx };
        auto offset = m_handleData.dataBuilder.addRange(std::forward<T_Data>(dataRange), range);
        m_rangeDirty.store(true, std::memory_order_release);
        return offset;
    }

    /**
     * @brief Commits all pending resource updates to the GPU
     * 
     * This method should be called after registering resources to ensure that:
     * 1. The handle buffer is updated on the GPU
     * 2. All pending descriptor updates are applied
     * 
     * Thread-safe: Uses synchronized access to multiple resources.
     */
    void build();

    /**
     * @brief Releases all resources and resets to initial state
     * 
     * Thread-safe: Uses comprehensive locking to ensure safe cleanup.
     */
    void clear();

    /**
     * @brief Generates Slang shader code for accessing bindless resources
     *
     * Creates structures and helper functions to access the registered resources
     * by name in shader code. The generated code can be included in shaders.
     *
     * Thread-safe: Uses shared lock to allow concurrent reads.
     *
     * @return Slang source code for bindless resource access
     */
    std::string generateHandleSource() const;

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
    enum ResourceType : uint32_t
    {
        eImage = 0,
        eBuffer = 1,
        eSampler = 2,
        eResourceTypeCount
    };

    HandleId updateResource(Buffer* pBuffer);
    HandleId updateResource(Image* pImage);
    HandleId updateResource(Sampler* pSampler);

private:
    Device* m_pDevice = {};

    // Handle data storage and management
    struct Handle
    {
        Handle(uint32_t minAlignment)
            : dataBuilder(minAlignment)
        {
        }

        DataBuilder dataBuilder;        // Builder for CPU-side handle data
        Buffer* pBuffer = {};           // GPU buffer containing handle data
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet = {};
    } m_handleData;

    // Resource data storage and management
    struct Resource
    {
        static constexpr std::size_t AddressTableSize = 4 * memory::KB;
        Buffer* pAddressTableBuffer = {};          // GPU buffer for buffer addresses
        std::span<uint64_t> addressTableMap;       // Mapped view of address table
        DescriptorSetLayout* pSetLayout = {};
        DescriptorSet* pSet = {};
    } m_resourceData;

    // Pipeline layout combining resource and handle sets
    PipelineLayout m_pipelineLayout{};

    // Flag indicating if handle data needs to be uploaded to GPU
    std::atomic<bool> m_rangeDirty{false};

    // Resources
    SmallVector<Image*> m_images;                // All registered images
    SmallVector<Buffer*> m_buffers;              // All registered buffers
    SmallVector<Sampler*> m_samplers;            // All registered samplers
    HashMap<Image*, HandleId> m_imageIds;
    HashMap<Buffer*, HandleId> m_bufferIds;
    HashMap<Sampler*, HandleId> m_samplerIds;

    HashMap<std::string, RType> m_handleNameMap; // Maps names to resources

    // Pending descriptor updates
    SmallVector<DescriptorUpdateInfo> m_resourceUpdateInfos;
    
    mutable std::mutex m_handleMtx;               // Protects handle data buffer
    mutable std::shared_mutex m_nameMtx;          // Protects name map (shared for reads)
    mutable std::shared_mutex m_resourceMapsMtx;  // Protects resource collections
    mutable std::mutex m_updateInfoMtx;           // Protects update info collection

};

} // namespace aph::vk
