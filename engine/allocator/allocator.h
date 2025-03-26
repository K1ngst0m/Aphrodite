#pragma once

#ifdef __cplusplus
#include <new>
#include <utility> // std::forward only
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "common/logger.h"
#include <source_location>
#include <stddef.h>

namespace aph::memory
{

constexpr std::size_t KB = 1024;
constexpr std::size_t MB = 1024 * KB;
constexpr std::size_t GB = 1024 * MB;

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
    MM_LOG_DEBUG("malloc: file={} line={} func={} size={}", location.file_name(), location.line(),
                 location.function_name(), size);

    return malloc_internal(size, location.file_name(), static_cast<int>(location.line()), location.function_name());
}

inline void* aph_memalign(std::size_t alignment, std::size_t size,
                          const std::source_location& location = std::source_location::current())
{
    MM_LOG_DEBUG("memalign: file={} line={} func={} alignment={} size={}", location.file_name(), location.line(),
                 location.function_name(), alignment, size);

    return memalign_internal(alignment, size, location.file_name(), static_cast<int>(location.line()),
                             location.function_name());
}

inline void* aph_calloc(std::size_t count, std::size_t size,
                        const std::source_location& location = std::source_location::current())
{
    MM_LOG_DEBUG("calloc: file={} line={} func={} count={} size={}", location.file_name(), location.line(),
                 location.function_name(), count, size);

    return calloc_internal(count, size, location.file_name(), static_cast<int>(location.line()),
                           location.function_name());
}

inline void* aph_calloc_memalign(std::size_t count, std::size_t alignment, std::size_t size,
                                 const std::source_location& location = std::source_location::current())
{
    MM_LOG_DEBUG("calloc_memalign: file={} line={} func={} count={} alignment={} size={}", location.file_name(),
                 location.line(), location.function_name(), count, alignment, size);

    return calloc_memalign_internal(count, alignment, size, location.file_name(), static_cast<int>(location.line()),
                                    location.function_name());
}

inline void* aph_realloc(void* ptr, std::size_t size,
                         const std::source_location& location = std::source_location::current())
{
    MM_LOG_DEBUG("realloc: file={} line={} func={} ptr={} size={}", location.file_name(), location.line(),
                 location.function_name(), ptr, size);

    return realloc_internal(ptr, size, location.file_name(), static_cast<int>(location.line()),
                            location.function_name());
}

inline void aph_free(void* ptr, const std::source_location& location = std::source_location::current())
{
    MM_LOG_DEBUG("free: file={} line={} func={} ptr={}", location.file_name(), location.line(),
                 location.function_name(), ptr);

    free_internal(ptr, location.file_name(), static_cast<int>(location.line()), location.function_name());
}

template <typename ObjectType, typename... Args>
ObjectType* aph_new(const std::source_location& location = std::source_location::current(), Args&&... args)
{
    MM_LOG_DEBUG("new: file={} line={} func={}", location.file_name(), location.line(), location.function_name());

    return new_internal<ObjectType>(location.file_name(), static_cast<int>(location.line()), location.function_name(),
                                    std::forward<Args>(args)...);
}

template <typename ObjectType>
void aph_delete(ObjectType* ptr, const std::source_location& location = std::source_location::current())
{
    MM_LOG_DEBUG("delete: file={} line={} func={} ptr={}", location.file_name(), location.line(),
                 location.function_name(), static_cast<void*>(ptr));

    delete_internal(ptr, location.file_name(), static_cast<int>(location.line()), location.function_name());
}

} // namespace aph::memory
