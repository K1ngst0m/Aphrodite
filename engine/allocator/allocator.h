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

#ifndef aph_malloc
    #define aph_malloc(size) malloc_internal(size, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif
#ifndef aph_memalign
    #define aph_memalign(align, size) memalign_internal(align, size, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif
#ifndef aph_calloc
    #define aph_calloc(count, size) calloc_internal(count, size, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif
#ifndef aph_calloc_memalign
    #define aph_calloc_memalign(count, align, size) \
        calloc_memalign_internal(count, align, size, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif
#ifndef aph_realloc
    #define aph_realloc(ptr, size) realloc_internal(ptr, size, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif
#ifndef aph_free
    #define aph_free(ptr) free_internal(ptr, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

#ifndef aph_new
    #define aph_new(ObjectType, ...) new_internal<ObjectType>(__FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef aph_delete
    #define aph_delete(ptr) delete_internal(ptr, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

}  // namespace aph::memory

#endif
