#include "allocator.h"
#include "global/globalManager.h"

#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <algorithm>
#include <vector>
#include <atomic>
#include <sstream>
#include <iomanip>

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
    // Check if GlobalManager is already initialized
    if (GlobalManager* globalManager = &aph::getGlobalManager())
    {
        return globalManager->getSubsystem<AllocationTracker>(GlobalManager::MEMORY_TRACKER_NAME);
    }
    return nullptr;
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
    std::ostringstream report;

    if (m_stats.empty())
    {
        report << "No memory allocations tracked\n";
        return report.str();
    }

    // Calculate summary statistics
    size_t totalAllocations = 0;
    size_t totalBytesAllocated = 0;
    size_t totalBytesDeallocated = 0;
    
    std::unordered_map<AllocationStat::Type, size_t> opCounts;
    std::unordered_map<AllocationStat::Type, size_t> opSizes;
    
    for (const auto& stat : m_stats)
    {
        opCounts[stat.type]++;
        
        if (stat.type == AllocationStat::Type::Free || 
            stat.type == AllocationStat::Type::Delete)
        {
            totalBytesDeallocated += stat.size;
        }
        else
        {
            totalAllocations++;
            totalBytesAllocated += stat.size;
            opSizes[stat.type] += stat.size;
        }
    }
    
    // Generate the summary report
    report << "\n===== Memory Allocation Summary =====\n";
    report << "Total allocations: " << totalAllocations << "\n";
    report << "Total bytes allocated: " << totalBytesAllocated 
           << " (" << std::fixed << std::setprecision(2) 
           << static_cast<double>(totalBytesAllocated) / MB << " MB)\n";
    report << "Total bytes deallocated: " << totalBytesDeallocated 
           << " (" << std::fixed << std::setprecision(2) 
           << static_cast<double>(totalBytesDeallocated) / MB << " MB)\n";
    report << "Current memory usage: " << (totalBytesAllocated - totalBytesDeallocated)
           << " (" << std::fixed << std::setprecision(2) 
           << static_cast<double>(totalBytesAllocated - totalBytesDeallocated) / MB << " MB)\n";
    
    report << "\n===== Operation Breakdown =====\n";
    
    static const char* TYPE_NAMES[] = {
        "Malloc", "Memalign", "Calloc", "CallocMemalign", 
        "Realloc", "Free", "New", "Delete"
    };
    
    for (size_t i = 0; i < 8; i++)
    {
        AllocationStat::Type type = static_cast<AllocationStat::Type>(i);
        
        if (opCounts.find(type) != opCounts.end())
        {
            auto count = opCounts.at(type);
            
            if (type == AllocationStat::Type::Free || type == AllocationStat::Type::Delete)
            {
                report << TYPE_NAMES[i] << ": " << count << " operations\n";
            }
            else if (opSizes.find(type) != opSizes.end())
            {
                auto size = opSizes.at(type);
                report << TYPE_NAMES[i] << ": " << count << " operations, " 
                       << size << " bytes (" 
                       << std::fixed << std::setprecision(2) << static_cast<double>(size) / KB << " KB)\n";
            }
        }
    }
    
    return report.str();
}

std::string AllocationTracker::generateFileReport() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream report;
    
    if (m_stats.empty())
    {
        report << "No memory allocations tracked\n";
        return report.str();
    }
    
    // Group by file
    std::unordered_map<std::string, size_t> fileAllocations;
    std::unordered_map<std::string, size_t> fileBytes;
    
    for (const auto& stat : m_stats)
    {
        if (stat.type != AllocationStat::Type::Free && 
            stat.type != AllocationStat::Type::Delete)
        {
            fileAllocations[stat.file]++;
            fileBytes[stat.file] += stat.size;
        }
    }
    
    report << "===== Allocations By File =====\n";
    
    // Convert to vector for sorting
    std::vector<std::pair<std::string, size_t>> sortedFiles;
    for (const auto& [file, count] : fileAllocations)
    {
        sortedFiles.push_back({file, count});
    }
    
    // Sort by allocation count (descending)
    std::sort(sortedFiles.begin(), sortedFiles.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [file, count] : sortedFiles)
    {
        report << file << ": " << count << " allocations, " 
               << fileBytes[file] << " bytes (" 
               << std::fixed << std::setprecision(2) 
               << static_cast<double>(fileBytes[file]) / KB << " KB)\n";
    }
    
    return report.str();
}

std::string AllocationTracker::generateLargestAllocationsReport(size_t count) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream report;
    
    if (m_stats.empty())
    {
        report << "No memory allocations tracked\n";
        return report.str();
    }
    
    // Copy allocations that are not frees or deletes
    std::vector<AllocationStat> allocations;
    for (const auto& stat : m_stats)
    {
        if (stat.type != AllocationStat::Type::Free && 
            stat.type != AllocationStat::Type::Delete)
        {
            allocations.push_back(stat);
        }
    }
    
    // Sort by size in descending order
    std::sort(allocations.begin(), allocations.end(),
             [](const AllocationStat& a, const AllocationStat& b) {
                 return a.size > b.size;
             });
    
    // Generate the report
    report << "===== Largest Allocations =====\n";
    for (size_t i = 0; i < std::min(count, allocations.size()); i++)
    {
        const auto& stat = allocations[i];
        report << (i + 1) << ". " << stat.size << " bytes at " << stat.ptr 
               << " - " << stat.file << ":" << stat.line 
               << " in " << stat.function << "\n";
    }
    
    return report.str();
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
