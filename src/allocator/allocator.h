#pragma once

#ifdef __cplusplus
#include <new>
#include <utility> // std::forward only
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "common/logger.h"
#include <mutex>
#include <source_location>
#include <stddef.h>
#include <string>

namespace aph::memory
{

constexpr std::size_t KB = 1024;
constexpr std::size_t MB = 1024 * KB;
constexpr std::size_t GB = 1024 * MB;

// Memory allocation statistics tracking
struct AllocationStat
{
    enum class Type
    {
        Malloc,
        Memalign,
        Calloc,
        CallocMemalign,
        Realloc,
        Free,
        New,
        Delete
    };

    Type type;
    std::string file;
    int line;
    std::string function;
    void* ptr;
    std::size_t size;
    std::size_t alignment;
    std::size_t count;
};

class AllocationTracker
{
public:
    AllocationTracker() = default;
    ~AllocationTracker() = default;

    void trackAllocation(const AllocationStat& stat);
    const SmallVector<AllocationStat>& getStats() const;
    void clear();

    // Generate string reports instead of directly logging
    std::string generateSummaryReport() const;
    std::string generateFileReport() const;
    std::string generateLargestAllocationsReport(size_t count = 10) const;

private:
    // Helper method to format bytes into human-readable sizes
    std::string formatSize(size_t bytes) const;

    std::mutex mutable m_mutex;
    SmallVector<AllocationStat> m_stats;
};

// Forward declaration for getting the allocation tracker from global manager
AllocationTracker* getActiveAllocationTracker();

void* malloc_internal(size_t size, const char* f, int l, const char* sf);
void* memalign_internal(size_t align, size_t size, const char* f, int l, const char* sf);
void* calloc_internal(size_t count, size_t size, const char* f, int l, const char* sf);
void* calloc_memalign_internal(size_t count, size_t align, size_t size, const char* f, int l, const char* sf);
void* realloc_internal(void* ptr, size_t size, const char* f, int l, const char* sf);
void free_internal(void* ptr, const char* f, int l, const char* sf);

template <typename T, typename... Args>
static T* placement_new(void* ptr, Args&&... args)
{
    return new (ptr) T(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
static T* new_internal(const char* f, int l, const char* sf, Args&&... args)
{
    T* ptr = static_cast<T*>(memalign_internal(alignof(T), sizeof(T), f, l, sf));
    return placement_new<T>(ptr, std::forward<Args>(args)...);
}

template <typename T>
static void delete_internal(T* ptr, const char* f, int l, const char* sf)
{
    if (ptr)
    {
        ptr->~T();
        free_internal(ptr, f, l, sf);
    }
}

inline void* aph_malloc(std::size_t size, const std::source_location& location = std::source_location::current())
{
    void* result =
        malloc_internal(size, location.file_name(), static_cast<int>(location.line()), location.function_name());

    // Track allocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::Malloc,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = result,
                                  .size = size,
                                  .alignment = 0,
                                  .count = 0});
    }

    return result;
}

inline void* aph_memalign(std::size_t alignment, std::size_t size,
                          const std::source_location& location = std::source_location::current())
{
    void* result = memalign_internal(alignment, size, location.file_name(), static_cast<int>(location.line()),
                                     location.function_name());

    // Track allocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::Memalign,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = result,
                                  .size = size,
                                  .alignment = alignment,
                                  .count = 0});
    }

    return result;
}

inline void* aph_calloc(std::size_t count, std::size_t size,
                        const std::source_location& location = std::source_location::current())
{
    void* result =
        calloc_internal(count, size, location.file_name(), static_cast<int>(location.line()), location.function_name());

    // Track allocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::Calloc,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = result,
                                  .size = size,
                                  .alignment = 0,
                                  .count = count});
    }

    return result;
}

inline void* aph_calloc_memalign(std::size_t count, std::size_t alignment, std::size_t size,
                                 const std::source_location& location = std::source_location::current())
{
    void* result = calloc_memalign_internal(count, alignment, size, location.file_name(),
                                            static_cast<int>(location.line()), location.function_name());

    // Track allocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::CallocMemalign,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = result,
                                  .size = size,
                                  .alignment = alignment,
                                  .count = count});
    }

    return result;
}

inline void* aph_realloc(void* ptr, std::size_t size,
                         const std::source_location& location = std::source_location::current())
{
    void* result =
        realloc_internal(ptr, size, location.file_name(), static_cast<int>(location.line()), location.function_name());

    // Track allocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::Realloc,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = result,
                                  .size = size,
                                  .alignment = 0,
                                  .count = 0});
    }

    return result;
}

inline void aph_free(void* ptr, const std::source_location& location = std::source_location::current())
{
    // Track deallocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::Free,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = ptr,
                                  .size = 0,
                                  .alignment = 0,
                                  .count = 0});
    }

    free_internal(ptr, location.file_name(), static_cast<int>(location.line()), location.function_name());
}

template <typename ObjectType, typename... Args>
ObjectType* aph_new(const std::source_location& location = std::source_location::current(), Args&&... args)
{
    ObjectType* result = new_internal<ObjectType>(location.file_name(), static_cast<int>(location.line()),
                                                  location.function_name(), std::forward<Args>(args)...);

    // Track allocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::New,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = result,
                                  .size = sizeof(ObjectType),
                                  .alignment = alignof(ObjectType),
                                  .count = 0});
    }

    return result;
}

template <typename ObjectType>
void aph_delete(ObjectType* ptr, const std::source_location& location = std::source_location::current())
{
    // Track deallocation if tracker is available
    if (auto tracker = getActiveAllocationTracker())
    {
        tracker->trackAllocation({.type = AllocationStat::Type::Delete,
                                  .file = location.file_name(),
                                  .line = static_cast<int>(location.line()),
                                  .function = location.function_name(),
                                  .ptr = static_cast<void*>(ptr),
                                  .size = sizeof(ObjectType),
                                  .alignment = alignof(ObjectType),
                                  .count = 0});
    }

    delete_internal(ptr, location.file_name(), static_cast<int>(location.line()), location.function_name());
}

} // namespace aph::memory
