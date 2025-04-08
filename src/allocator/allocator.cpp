#include "allocator.h"
#include "common/hash.h"
#include "global/globalManager.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <malloc.h>
#include <memory>
#include <sstream>
#include <vector>

namespace
{
template <std::integral T>
constexpr T alignTo(T size, T alignment) noexcept
{
    return ((size + alignment - 1) & ~(alignment - 1));
}
} // namespace

namespace aph::memory
{

// Retrieve the allocation tracker from the global manager if available
AllocationTracker* getActiveAllocationTracker()
{
    return APH_GLOBAL_MANAGER.getSubsystem<AllocationTracker>(GlobalManager::MEMORY_TRACKER_NAME);
}

void AllocationTracker::trackAllocation(const AllocationStat& stat)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.push_back(stat);
}

const SmallVector<AllocationStat>& AllocationTracker::getStats() const
{
    return m_stats;
}

void AllocationTracker::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.clear();
}

std::string AllocationTracker::generateSummaryReport() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;

    // Counters for allocation types
    size_t totalAllocations = 0;
    size_t totalDeallocations = 0;
    size_t totalBytesAllocated = 0;

    // Track active (non-freed) allocations
    HashMap<void*, AllocationStat> activeAllocations;

    // Process all allocation events
    for (const auto& stat : m_stats)
    {
        switch (stat.type)
        {
        case AllocationStat::Type::Malloc:
        case AllocationStat::Type::Memalign:
        case AllocationStat::Type::Calloc:
        case AllocationStat::Type::CallocMemalign:
        case AllocationStat::Type::New:
            totalAllocations++;
            totalBytesAllocated += stat.size;
            activeAllocations[stat.ptr] = stat;
            break;

        case AllocationStat::Type::Realloc:
            // Handle realloc as a special case - could be new or replacement
            if (activeAllocations.find(stat.ptr) == activeAllocations.end())
            {
                totalAllocations++;
                totalBytesAllocated += stat.size;
            }
            else
            {
                // Adjust total bytes for the size difference
                totalBytesAllocated = totalBytesAllocated - activeAllocations[stat.ptr].size + stat.size;
            }
            activeAllocations[stat.ptr] = stat;
            break;

        case AllocationStat::Type::Free:
        case AllocationStat::Type::Delete:
            totalDeallocations++;
            activeAllocations.erase(stat.ptr);
            break;
        }
    }

    // Calculate currently allocated memory
    size_t currentlyAllocated = 0;
    for (const auto& [ptr, stat] : activeAllocations)
    {
        currentlyAllocated += stat.size;
    }

    // Format the summary report
    ss << "\n===============================================\n";
    ss << "MEMORY ALLOCATION SUMMARY\n";
    ss << "===============================================\n";
    ss << "Total allocations:    " << totalAllocations << "\n";
    ss << "Total deallocations:  " << totalDeallocations << "\n";
    ss << "Outstanding calls:    " << (totalAllocations - totalDeallocations) << "\n";
    ss << "Total bytes allocated: " << formatSize(totalBytesAllocated) << "\n";
    ss << "Current memory usage:  " << formatSize(currentlyAllocated) << "\n";
    ss << "Outstanding allocations: " << activeAllocations.size() << "\n";
    ss << "===============================================\n";

    // Add potential leak information
    if (!activeAllocations.empty())
    {
        ss << "\nPOTENTIAL MEMORY LEAKS:\n";
        ss << "-----------------------------------------------\n";
        ss << "Ptr       | Size     | Location\n";
        ss << "-----------------------------------------------\n";

        // Sort by size (largest first)
        std::vector<std::pair<void*, AllocationStat>> leaks;
        for (const auto& pair : activeAllocations)
        {
            leaks.push_back(pair);
        }

        std::sort(leaks.begin(), leaks.end(),
                  [](const auto& a, const auto& b) { return a.second.size > b.second.size; });

        // Show top 10 largest leaks
        int count = 0;
        for (const auto& [ptr, stat] : leaks)
        {
            ss << ptr << " | " << formatSize(stat.size) << " | " << stat.file << ":" << stat.line << " ("
               << stat.function << ")\n";

            if (++count >= 10)
                break;
        }

        if (leaks.size() > 10)
        {
            ss << "... and " << (leaks.size() - 10) << " more\n";
        }
        ss << "-----------------------------------------------\n";
    }

    return ss.str();
}

std::string AllocationTracker::generateFileReport() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;

    // Group allocations by file
    HashMap<std::string, size_t> fileAllocations;
    HashMap<std::string, size_t> fileBytes;

    // Track active (non-freed) allocations
    HashMap<void*, AllocationStat> activeAllocations;

    // Process all allocation events
    for (const auto& stat : m_stats)
    {
        switch (stat.type)
        {
        case AllocationStat::Type::Malloc:
        case AllocationStat::Type::Memalign:
        case AllocationStat::Type::Calloc:
        case AllocationStat::Type::CallocMemalign:
        case AllocationStat::Type::New:
        case AllocationStat::Type::Realloc:
            activeAllocations[stat.ptr] = stat;
            break;

        case AllocationStat::Type::Free:
        case AllocationStat::Type::Delete:
            activeAllocations.erase(stat.ptr);
            break;
        }
    }

    // Count active allocations by file
    for (const auto& [ptr, stat] : activeAllocations)
    {
        fileAllocations[stat.file]++;
        fileBytes[stat.file] += stat.size;
    }

    // Sort files by total bytes (largest first)
    std::vector<std::pair<std::string, size_t>> sortedFiles;
    for (const auto& pair : fileBytes)
    {
        sortedFiles.push_back(pair);
    }

    std::sort(sortedFiles.begin(), sortedFiles.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    // Format the file report
    ss << "===============================================\n";
    ss << "MEMORY ALLOCATION BY FILE\n";
    ss << "===============================================\n";
    ss << "File                  | Count | Size\n";
    ss << "-----------------------------------------------\n";

    for (const auto& [file, bytes] : sortedFiles)
    {
        ss << file << " | " << fileAllocations[file] << " | " << formatSize(bytes) << "\n";
    }

    ss << "-----------------------------------------------\n";
    ss << "Total: " << activeAllocations.size() << " allocations\n";
    ss << "===============================================\n";

    return ss.str();
}

std::string AllocationTracker::generateLargestAllocationsReport(size_t count) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;

    // Track active (non-freed) allocations
    HashMap<void*, AllocationStat> activeAllocations;

    // Process all allocation events
    for (const auto& stat : m_stats)
    {
        switch (stat.type)
        {
        case AllocationStat::Type::Malloc:
        case AllocationStat::Type::Memalign:
        case AllocationStat::Type::Calloc:
        case AllocationStat::Type::CallocMemalign:
        case AllocationStat::Type::New:
        case AllocationStat::Type::Realloc:
            activeAllocations[stat.ptr] = stat;
            break;

        case AllocationStat::Type::Free:
        case AllocationStat::Type::Delete:
            activeAllocations.erase(stat.ptr);
            break;
        }
    }

    // Sort allocations by size (largest first)
    std::vector<std::pair<void*, AllocationStat>> sortedAllocations;
    for (const auto& pair : activeAllocations)
    {
        sortedAllocations.push_back(pair);
    }

    std::sort(sortedAllocations.begin(), sortedAllocations.end(),
              [](const auto& a, const auto& b) { return a.second.size > b.second.size; });

    // Format the largest allocations report
    ss << "===============================================\n";
    ss << "LARGEST ACTIVE ALLOCATIONS\n";
    ss << "===============================================\n";
    ss << "Ptr       | Size     | Location\n";
    ss << "-----------------------------------------------\n";

    size_t displayCount = std::min(count, sortedAllocations.size());
    for (size_t i = 0; i < displayCount; i++)
    {
        const auto& [ptr, stat] = sortedAllocations[i];
        ss << ptr << " | " << formatSize(stat.size) << " | " << stat.file << ":" << stat.line << " (" << stat.function
           << ")\n";
    }

    ss << "-----------------------------------------------\n";
    ss << "Total active allocations: " << activeAllocations.size() << "\n";
    ss << "===============================================\n";

    return ss.str();
}

// Helper method to format bytes into human-readable sizes
std::string AllocationTracker::formatSize(size_t bytes) const
{
    std::stringstream ss;
    if (bytes < KB)
    {
        ss << bytes << " B";
    }
    else if (bytes < MB)
    {
        ss << (bytes / static_cast<double>(KB)) << " KB";
    }
    else if (bytes < GB)
    {
        ss << (bytes / static_cast<double>(MB)) << " MB";
    }
    else
    {
        ss << (bytes / static_cast<double>(GB)) << " GB";
    }
    return ss.str();
}

// Original allocation function implementations
void* malloc_internal(size_t size, const char* f, int l, const char* sf)
{
    return std::malloc(size);
}

void* memalign_internal(size_t align, size_t size, const char* f, int l, const char* sf)
{
    size_t alignedSize = alignTo(size, align);
    return std::aligned_alloc(align, alignedSize);
}

void* calloc_internal(size_t count, size_t size, const char* f, int l, const char* sf)
{
    return std::calloc(count, size);
}

void* calloc_memalign(size_t count, size_t alignment, size_t size)
{
    size_t alignedArrayElementSize = alignTo(size, alignment);
    size_t totalBytes = count * alignedArrayElementSize;

    void* ptr = memalign(alignment, totalBytes);

    std::memset(ptr, 0, totalBytes);
    return ptr;
}

void* calloc_memalign_internal(size_t count, size_t align, size_t size, const char* f, int l, const char* sf)
{
    return calloc_memalign(count, align, size);
}

void* realloc_internal(void* ptr, size_t size, const char* f, int l, const char* sf)
{
    return std::realloc(ptr, size);
}

void free_internal(void* ptr, const char* f, int l, const char* sf)
{
    std::free(ptr);
}
} // namespace aph::memory
