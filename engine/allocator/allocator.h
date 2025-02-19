#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#ifdef __cplusplus
    #include <new>
    #include <utility>  // std::forward only
#else
    #include <stdint.h>
    #include <stdbool.h>
#endif

#include <stddef.h>
#include <source_location>

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
void  free_internal(void* ptr, const char* f, int l, const char* sf);

template <typename T, typename... Args>
static T* placement_new(void* ptr, Args&&... args)
{
    return new(ptr) T(std::forward<Args>(args)...);
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
    if(ptr)
    {
        ptr->~T();
        free_internal(ptr, f, l, sf);
    }
}

inline void* aph_malloc(
    std::size_t size,
    const std::source_location& location = std::source_location::current())
{
    return malloc_internal(size,
                           location.file_name(),
                           static_cast<int>(location.line()),
                           location.function_name());
}

inline void* aph_memalign(
    std::size_t alignment,
    std::size_t size,
    const std::source_location& location = std::source_location::current())
{
    return memalign_internal(alignment,
                             size,
                             location.file_name(),
                             static_cast<int>(location.line()),
                             location.function_name());
}

inline void* aph_calloc(
    std::size_t count,
    std::size_t size,
    const std::source_location& location = std::source_location::current())
{
    return calloc_internal(count,
                           size,
                           location.file_name(),
                           static_cast<int>(location.line()),
                           location.function_name());
}

inline void* aph_calloc_memalign(
    std::size_t count,
    std::size_t alignment,
    std::size_t size,
    const std::source_location& location = std::source_location::current())
{
    return calloc_memalign_internal(count,
                                    alignment,
                                    size,
                                    location.file_name(),
                                    static_cast<int>(location.line()),
                                    location.function_name());
}

inline void* aph_realloc(
    void* ptr,
    std::size_t size,
    const std::source_location& location = std::source_location::current())
{
    return realloc_internal(ptr,
                            size,
                            location.file_name(),
                            static_cast<int>(location.line()),
                            location.function_name());
}

inline void aph_free(
    void* ptr,
    const std::source_location& location = std::source_location::current())
{
    free_internal(ptr,
                  location.file_name(),
                  static_cast<int>(location.line()),
                  location.function_name());
}

template <typename ObjectType, typename... Args>
ObjectType* aph_new(
    const std::source_location& location = std::source_location::current(),
    Args&&... args)
{
    return new_internal<ObjectType>(
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name(),
        std::forward<Args>(args)...
    );
}

template <typename ObjectType>
void aph_delete(
    ObjectType* ptr,
    const std::source_location& location = std::source_location::current())
{
    delete_internal(
        ptr,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name()
    );
}

}  // namespace aph::memory

#endif
