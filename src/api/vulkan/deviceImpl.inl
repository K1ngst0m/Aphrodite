// Implementation file for Device template methods
template <>
struct ResourceTraits<BufferCreateInfo>
{
    using ResourceType = Buffer;
};

template <>
struct ResourceTraits<ImageCreateInfo>
{
    using ResourceType = Image;
};

template <>
struct ResourceTraits<ImageViewCreateInfo>
{
    using ResourceType = ImageView;
};

template <>
struct ResourceTraits<SamplerCreateInfo>
{
    using ResourceType = Sampler;
};

template <>
struct ResourceTraits<ProgramCreateInfo>
{
    using ResourceType = ShaderProgram;
};

template <>
struct ResourceTraits<DescriptorSetLayoutCreateInfo>
{
    using ResourceType = DescriptorSetLayout;
};

template <>
struct ResourceTraits<PipelineLayoutCreateInfo>
{
    using ResourceType = PipelineLayout;
};

template <>
struct ResourceTraits<SwapChainCreateInfo>
{
    using ResourceType = SwapChain;
};

// ResourceStats implementation
inline auto ResourceStats::resourceTypeToString(ResourceType type) -> const char*
{
    static const char* typeNames[] = {
        "Buffer",
        "Image",
        "ImageView",
        "Sampler",
        "ShaderProgram",
        "DescriptorSetLayout",
        "PipelineLayout",
        "SwapChain",
        "CommandBuffer",
        "Queue",
        "Fence",
        "Semaphore"
    };
    
    return typeNames[static_cast<size_t>(type)];
}

inline auto ResourceStats::trackCreation(ResourceType type) -> void
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_created[static_cast<size_t>(type)]++;
    m_active[static_cast<size_t>(type)]++;
}

inline auto ResourceStats::trackDestruction(ResourceType type) -> void
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_destroyed[static_cast<size_t>(type)]++;
    m_active[static_cast<size_t>(type)]--;
}

inline auto ResourceStats::generateReport() const -> std::string
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;
    
    ss << "Resource Usage Report:\n";
    ss << "--------------------------------------------------\n";
    ss << "Type                 | Created | Destroyed | Active\n";
    ss << "--------------------------------------------------\n";
    
    bool hasLeaks = false;
    std::stringstream leakReport;
    leakReport << "\nPotential Resource Leaks:\n";
    leakReport << "--------------------------------------------------\n";
    leakReport << "Type                 | Leaked | % of Created\n";
    leakReport << "--------------------------------------------------\n";
    
    uint32_t totalCreated = 0;
    uint32_t totalDestroyed = 0;
    uint32_t totalLeaked = 0;
    
    for (size_t i = 0; i < static_cast<size_t>(ResourceType::eCount); i++)
    {
        auto type = static_cast<ResourceType>(i);
        uint32_t created = m_created[i];
        uint32_t destroyed = m_destroyed[i];
        uint32_t active = m_active[i];
        
        // Add to totals
        totalCreated += created;
        totalDestroyed += destroyed;
        
        // Output standard resource report
        ss << std::left << std::setw(20) << resourceTypeToString(type) << " | "
           << std::right << std::setw(7) << created << " | "
           << std::setw(9) << destroyed << " | "
           << std::setw(6) << active << "\n";
        
        // Check for leaks and add to leak report if found
        if (active > 0 && created > 0)
        {
            hasLeaks = true;
            totalLeaked += active;
            double leakPercentage = (static_cast<double>(active) / created) * 100.0;
            
            leakReport << std::left << std::setw(20) << resourceTypeToString(type) << " | "
                       << std::right << std::setw(6) << active << " | "
                       << std::fixed << std::setprecision(1) << std::setw(6) << leakPercentage << "%\n";
        }
    }
    
    ss << "--------------------------------------------------\n";
    ss << "Total                | " 
       << std::right << std::setw(7) << totalCreated << " | "
       << std::setw(9) << totalDestroyed << " | "
       << std::setw(6) << totalLeaked << "\n";
    ss << "--------------------------------------------------\n";
    
    // Add leak report if leaks were found
    if (hasLeaks)
    {
        double overallLeakPercentage = (static_cast<double>(totalLeaked) / totalCreated) * 100.0;
        
        leakReport << "--------------------------------------------------\n";
        leakReport << "Total Resources Leaked: " << totalLeaked << " (" 
                   << std::fixed << std::setprecision(1) << overallLeakPercentage 
                   << "% of all created resources)\n";
        leakReport << "--------------------------------------------------\n";
        
        ss << "\n" << leakReport.str();
    }
    else
    {
        ss << "\nNo Resource Leaks Detected!\n";
    }
    
    return ss.str();
}

inline auto ResourceStats::getCreatedCount(ResourceType type) const -> uint32_t
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_created[static_cast<size_t>(type)]; 
}

inline auto ResourceStats::getDestroyedCount(ResourceType type) const -> uint32_t
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_destroyed[static_cast<size_t>(type)]; 
}

inline auto ResourceStats::getActiveCount(ResourceType type) const -> uint32_t
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active[static_cast<size_t>(type)]; 
}


template <typename T>
inline auto getResourceType() -> ResourceStats::ResourceType
{
    static_assert(dependent_false_v<T>, "unsupported resource type.");
}

// Template specializations for resource type mapping
template<> inline auto getResourceType<Buffer>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eBuffer; }
template<> inline auto getResourceType<Image>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eImage; }
template<> inline auto getResourceType<ImageView>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eImageView; }
template<> inline auto getResourceType<Sampler>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eSampler; }
template<> inline auto getResourceType<ShaderProgram>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eShaderProgram; }
template<> inline auto getResourceType<DescriptorSetLayout>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eDescriptorSetLayout; }
template<> inline auto getResourceType<PipelineLayout>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::ePipelineLayout; }
template<> inline auto getResourceType<SwapChain>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eSwapChain; }
template<> inline auto getResourceType<CommandBuffer>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eCommandBuffer; }
template<> inline auto getResourceType<Queue>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eQueue; }
template<> inline auto getResourceType<Fence>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eFence; }
template<> inline auto getResourceType<Semaphore>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eSemaphore; }

// Resource tracking templates
template <typename TResource>
inline auto Device::trackResourceCreation() -> void
{
    m_resourceStats.trackCreation(getResourceType<TResource>());
}

template <typename TResource>
inline auto Device::trackResourceDestruction() -> void
{
    m_resourceStats.trackDestruction(getResourceType<TResource>());
}

// Main resource handling templates
template <typename TCreateInfo, typename TResource, typename TDebugName>
inline auto Device::create(TCreateInfo&& createInfo, TDebugName&& debugName,
                           const std::source_location& location) -> Expected<TResource*>
{
    auto result = createImpl(APH_FWD(createInfo));
    
    if (result.success())
    {
        // Get name if possible
        std::string name;
        if constexpr(std::is_convertible_v<TDebugName, std::string>)
        {
            name = std::forward<TDebugName>(debugName);
        }
        
        // Set object debug name if provided
        if (!name.empty())
        {
            APH_VERIFY_RESULT(setDebugObjectName(result.value(), name));
        }
        
        // Track resource creation in stats
        trackResourceCreation<TResource>();
    }
    
    return result;
}

template <typename TResource>
inline auto Device::destroy(TResource* pResource, const std::source_location& location) -> void
{
    if (pResource)
    {
        // Track resource destruction in stats
        trackResourceDestruction<TResource>();
        
        // Call implementation-specific destroy method
        destroyImpl(pResource);
    }
}

// Debug name setter templates
template <ResourceHandleType TObject>
inline auto Device::setDebugObjectName(TObject* object, auto&& name) -> Result
{
    object->setDebugName(APH_FWD(name));
    auto handle = object->getHandle();
    if constexpr (!std::is_same_v<DummyHandle, decltype(handle)>)
    {
        return setDebugObjectName(handle, object->getDebugName());
    }
    return Result::Success;
}

template <typename TObject>
    requires(!ResourceHandleType<TObject>)
inline auto Device::setDebugObjectName(TObject object, std::string_view name) -> Result
{
    ::vk::DebugUtilsObjectNameInfoEXT info{};
    info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
        .setObjectType(object.objectType)
        .setPObjectName(name.data());
    return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
}

inline auto Device::getResourceStatsReport() const -> std::string
{
    return m_resourceStats.generateReport();
}
