#pragma once

#include "common/common.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include <format>
#include <source_location>
#include <vector>

namespace aph::vk
{

// Resource statistics tracking
class ResourceStats
{
public:
    enum class ResourceType : uint8_t
    {
        eBuffer,
        eImage,
        eImageView,
        eSampler,
        eShaderProgram,
        eDescriptorSetLayout,
        ePipelineLayout,
        eSwapChain,
        eCommandBuffer,
        eQueue,
        eFence,
        eSemaphore,
        eQueryPool,
        eCount
    };

    // Source location tracking information
    struct LocationInfo
    {
        std::string file;
        uint32_t line;
        uint32_t count;
    };

    // Public interface
    auto generateReport() const -> std::string;
    auto getCreatedCount(ResourceType type) const -> uint32_t;
    auto getDestroyedCount(ResourceType type) const -> uint32_t;
    auto getActiveCount(ResourceType type) const -> uint32_t;

    // Resource tracking
    template <typename TResource>
    void trackCreation(const std::source_location& location = std::source_location::current());
    template <typename TResource>
    void trackDestruction(const std::source_location& location = std::source_location::current());

    // Enum utilities
    static auto resourceTypeToString(ResourceType type) -> const char*;
    static auto begin() -> ResourceType;
    static auto end() -> ResourceType;
    static auto next(ResourceType type) -> ResourceType;

private:
    mutable std::mutex m_mutex;
    HashMap<ResourceType, uint32_t> m_created;
    HashMap<ResourceType, uint32_t> m_destroyed;
    HashMap<ResourceType, uint32_t> m_active;
    
    // Source location tracking
    HashMap<ResourceType, SmallVector<LocationInfo>> m_creationLocations;
    HashMap<ResourceType, SmallVector<LocationInfo>> m_destructionLocations;
};

inline auto ResourceStats::begin() -> ResourceType
{
    return ResourceType::eBuffer;
}

inline auto ResourceStats::end() -> ResourceType
{
    return ResourceType::eCount;
}

inline auto ResourceStats::next(ResourceType type) -> ResourceType
{
    return ResourceType(static_cast<uint8_t>(type) + 1);
}

// ResourceStats implementation
inline auto ResourceStats::resourceTypeToString(ResourceType type) -> const char*
{
    static constexpr std::array kTypeNames = { "Buffer",         "Image",         "ImageView",
                                               "Sampler",        "ShaderProgram", "DescriptorSetLayout",
                                               "PipelineLayout", "SwapChain",     "CommandBuffer",
                                               "Queue",          "Fence",         "Semaphore",
                                               "QueryPool" };
    APH_ASSERT(static_cast<size_t>(type) < kTypeNames.size(), "Resource type out of bounds");
    return kTypeNames[static_cast<size_t>(type)];
}

template <typename T>
inline auto GetResourceType() -> ResourceStats::ResourceType
{
    static_assert(dependent_false_v<T>, "unsupported resource type.");
}

template <typename TResource>
inline void ResourceStats::trackCreation(const std::source_location& location)
{
    auto resType = GetResourceType<TResource>();
    std::lock_guard<std::mutex> lock(m_mutex);

    m_created[resType]++;
    m_active[resType]++;

    // Track source location
    std::string filePath = location.file_name();
    // Get filename without path
    size_t lastSlash = filePath.find_last_of("/\\");
    std::string fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
    
    auto& locations = m_creationLocations[resType];
    
    // Look for existing location entry
    auto it = std::find_if(locations.begin(), locations.end(), [&](const LocationInfo& info) {
        return info.file == fileName && info.line == location.line();
    });
    
    if (it != locations.end()) {
        // Increment existing location count
        it->count++;
    } else {
        // Add new location
        locations.push_back({ fileName, location.line(), 1 });
    }
}

template <typename TResource>
inline void ResourceStats::trackDestruction(const std::source_location& location)
{
    auto resType = GetResourceType<TResource>();
    std::lock_guard<std::mutex> lock(m_mutex);

    m_destroyed[resType]++;
    m_active[resType]--;

    // Track source location
    std::string filePath = location.file_name();
    // Get filename without path
    size_t lastSlash = filePath.find_last_of("/\\");
    std::string fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
    
    auto& locations = m_destructionLocations[resType];
    
    // Look for existing location entry
    auto it = std::find_if(locations.begin(), locations.end(), [&](const LocationInfo& info) {
        return info.file == fileName && info.line == location.line();
    });
    
    if (it != locations.end()) {
        // Increment existing location count
        it->count++;
    } else {
        // Add new location
        locations.push_back({ fileName, location.line(), 1 });
    }
}

inline auto ResourceStats::generateReport() const -> std::string
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string report = std::format("Resource Usage Report:\n"
                                     "--------------------------------------------------\n"
                                     "Type                 | Created | Destroyed | Active\n"
                                     "--------------------------------------------------\n");

    bool hasLeaks          = false;
    std::string leakReport = std::format("\nPotential Resource Leaks:\n"
                                         "--------------------------------------------------\n"
                                         "Type                 | Leaked | % of Created\n"
                                         "--------------------------------------------------\n");

    uint32_t totalCreated   = 0;
    uint32_t totalDestroyed = 0;
    uint32_t totalLeaked    = 0;

    // Iterate through all resource types using the helper functions
    for (auto resourceType = begin(); resourceType != end(); resourceType = next(resourceType))
    {
        uint32_t created   = m_created.contains(resourceType) ? m_created.at(resourceType) : 0;
        uint32_t destroyed = m_destroyed.contains(resourceType) ? m_destroyed.at(resourceType) : 0;
        uint32_t active    = m_active.contains(resourceType) ? m_active.at(resourceType) : 0;

        // Add to totals
        totalCreated += created;
        totalDestroyed += destroyed;

        // Output standard resource report
        report += std::format("{:<20} | {:>7} | {:>9} | {:>6}\n", resourceTypeToString(resourceType), created,
                              destroyed, active);

        // Check for leaks and add to leak report if found
        if (active > 0 && created > 0)
        {
            hasLeaks = true;
            totalLeaked += active;
            double leakPercentage = (static_cast<double>(active) / created) * 100.0;

            leakReport +=
                std::format("{:<20} | {:>6} | {:>6.1f}%\n", resourceTypeToString(resourceType), active, leakPercentage);
                
            // Add source location information for leaked resources
            if (m_creationLocations.contains(resourceType))
            {
                leakReport += std::format("  Creation locations:\n");
                auto& locations = m_creationLocations.at(resourceType);
                // Sort by count in descending order
                SmallVector<LocationInfo> sortedLocations = locations;
                std::sort(sortedLocations.begin(), sortedLocations.end(), 
                          [](const LocationInfo& a, const LocationInfo& b) { return a.count > b.count; });
                
                // Show top 5 creation locations
                for (size_t i = 0; i < std::min(size_t(5), sortedLocations.size()); ++i)
                {
                    const auto& info = sortedLocations[i];
                    leakReport += std::format("    {}:{} - {} instances\n", info.file, info.line, info.count);
                }
            }
        }
    }

    report += std::format("--------------------------------------------------\n"
                          "{:<20} | {:>7} | {:>9} | {:>6}\n"
                          "--------------------------------------------------\n",
                          "Total", totalCreated, totalDestroyed, totalLeaked);

    // Add leak report if leaks were found
    if (hasLeaks)
    {
        double overallLeakPercentage = (static_cast<double>(totalLeaked) / totalCreated) * 100.0;

        leakReport += std::format("--------------------------------------------------\n"
                                  "Total Resources Leaked: {} ({:.1f}% of all created resources)\n"
                                  "--------------------------------------------------\n",
                                  totalLeaked, overallLeakPercentage);

        report += leakReport;
    }
    else
    {
        report += "\nNo Resource Leaks Detected!\n";
    }

    return report;
}

inline auto ResourceStats::getCreatedCount(ResourceType type) const -> uint32_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_created.contains(type) ? m_created.at(type) : 0;
}

inline auto ResourceStats::getDestroyedCount(ResourceType type) const -> uint32_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_destroyed.contains(type) ? m_destroyed.at(type) : 0;
}

inline auto ResourceStats::getActiveCount(ResourceType type) const -> uint32_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active.contains(type) ? m_active.at(type) : 0;
}

// Forward declarations for resource type specializations
class Buffer;
class Image;
class ImageView;
class Sampler;
class ShaderProgram;
class DescriptorSetLayout;
class PipelineLayout;
class SwapChain;
class CommandBuffer;
class Queue;
class Fence;
class Semaphore;

// Template specializations for resource type mapping
template <>
inline auto GetResourceType<Buffer>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eBuffer;
}

template <>
inline auto GetResourceType<Image>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eImage;
}

template <>
inline auto GetResourceType<ImageView>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eImageView;
}

template <>
inline auto GetResourceType<Sampler>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eSampler;
}

template <>
inline auto GetResourceType<ShaderProgram>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eShaderProgram;
}

template <>
inline auto GetResourceType<DescriptorSetLayout>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eDescriptorSetLayout;
}

template <>
inline auto GetResourceType<PipelineLayout>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::ePipelineLayout;
}

template <>
inline auto GetResourceType<SwapChain>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eSwapChain;
}

template <>
inline auto GetResourceType<CommandBuffer>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eCommandBuffer;
}

template <>
inline auto GetResourceType<Queue>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eQueue;
}

template <>
inline auto GetResourceType<Fence>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eFence;
}

template <>
inline auto GetResourceType<Semaphore>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eSemaphore;
}

template <>
inline auto GetResourceType<QueryPool>() -> ResourceStats::ResourceType
{
    return ResourceStats::ResourceType::eQueryPool;
}

} // namespace aph::vk
