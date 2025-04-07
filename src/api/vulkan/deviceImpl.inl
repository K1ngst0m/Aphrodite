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
inline const char* ResourceStats::ResourceTypeToString(ResourceType type)
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

inline void ResourceStats::trackCreation(ResourceType type)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_created[static_cast<size_t>(type)]++;
    m_active[static_cast<size_t>(type)]++;
}

inline void ResourceStats::trackDestruction(ResourceType type)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_destroyed[static_cast<size_t>(type)]++;
    m_active[static_cast<size_t>(type)]--;
}

inline std::string ResourceStats::generateReport() const
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
    
    for (size_t i = 0; i < static_cast<size_t>(ResourceType::Count); i++)
    {
        auto type = static_cast<ResourceType>(i);
        uint32_t created = m_created[i];
        uint32_t destroyed = m_destroyed[i];
        uint32_t active = m_active[i];
        
        // Add to totals
        totalCreated += created;
        totalDestroyed += destroyed;
        
        // Output standard resource report
        ss << std::left << std::setw(20) << ResourceTypeToString(type) << " | "
           << std::right << std::setw(7) << created << " | "
           << std::setw(9) << destroyed << " | "
           << std::setw(6) << active << "\n";
        
        // Check for leaks and add to leak report if found
        if (active > 0 && created > 0)
        {
            hasLeaks = true;
            totalLeaked += active;
            double leakPercentage = (static_cast<double>(active) / created) * 100.0;
            
            leakReport << std::left << std::setw(20) << ResourceTypeToString(type) << " | "
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

inline uint32_t ResourceStats::getCreatedCount(ResourceType type) const 
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_created[static_cast<size_t>(type)]; 
}

inline uint32_t ResourceStats::getDestroyedCount(ResourceType type) const 
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_destroyed[static_cast<size_t>(type)]; 
}

inline uint32_t ResourceStats::getActiveCount(ResourceType type) const 
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active[static_cast<size_t>(type)]; 
}

// Template specializations for resource type mapping
template<> inline ResourceStats::ResourceType Device::getResourceType<Buffer>() const { return ResourceStats::ResourceType::Buffer; }
template<> inline ResourceStats::ResourceType Device::getResourceType<Image>() const { return ResourceStats::ResourceType::Image; }
template<> inline ResourceStats::ResourceType Device::getResourceType<ImageView>() const { return ResourceStats::ResourceType::ImageView; }
template<> inline ResourceStats::ResourceType Device::getResourceType<Sampler>() const { return ResourceStats::ResourceType::Sampler; }
template<> inline ResourceStats::ResourceType Device::getResourceType<ShaderProgram>() const { return ResourceStats::ResourceType::ShaderProgram; }
template<> inline ResourceStats::ResourceType Device::getResourceType<DescriptorSetLayout>() const { return ResourceStats::ResourceType::DescriptorSetLayout; }
template<> inline ResourceStats::ResourceType Device::getResourceType<PipelineLayout>() const { return ResourceStats::ResourceType::PipelineLayout; }
template<> inline ResourceStats::ResourceType Device::getResourceType<SwapChain>() const { return ResourceStats::ResourceType::SwapChain; }
template<> inline ResourceStats::ResourceType Device::getResourceType<CommandBuffer>() const { return ResourceStats::ResourceType::CommandBuffer; }
template<> inline ResourceStats::ResourceType Device::getResourceType<Queue>() const { return ResourceStats::ResourceType::Queue; }
template<> inline ResourceStats::ResourceType Device::getResourceType<Fence>() const { return ResourceStats::ResourceType::Fence; }
template<> inline ResourceStats::ResourceType Device::getResourceType<Semaphore>() const { return ResourceStats::ResourceType::Semaphore; }

// Resource tracking templates
template <typename TResource>
inline void Device::trackResourceCreation()
{
    m_resourceStats.trackCreation(getResourceType<TResource>());
}

template <typename TResource>
inline void Device::trackResourceDestruction()
{
    m_resourceStats.trackDestruction(getResourceType<TResource>());
}

// Main resource handling templates
template <typename TCreateInfo, typename TResource, typename TDebugName>
inline Expected<TResource*> Device::create(TCreateInfo&& createInfo, TDebugName&& debugName,
                                         const std::source_location& location)
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
inline void Device::destroy(TResource* pResource, const std::source_location& location)
{
    if (pResource)
    {
        // Track resource destruction in stats
        trackResourceDestruction<TResource>();
        
        destroyImpl(pResource);
    }
}

// Debug name setter templates
template <ResourceHandleType TObject>
inline Result Device::setDebugObjectName(TObject* object, auto&& name)
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
inline Result Device::setDebugObjectName(TObject object, std::string_view name)
{
    ::vk::DebugUtilsObjectNameInfoEXT info{};
    info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
        .setObjectType(object.objectType)
        .setPObjectName(name.data());
    return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
}

// Other inline implementations
inline std::string Device::getResourceStatsReport() const
{
    return m_resourceStats.generateReport();
} 