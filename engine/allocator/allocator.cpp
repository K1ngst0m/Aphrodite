#include "allocator.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

#if INTPTR_MAX == 0x7FFFFFFFFFFFFFFFLL
    #define PTR_SIZE 8
#elif INTPTR_MAX == 0x7FFFFFFF
    #define PTR_SIZE 4
#else
    #error unsupported platform
#endif

#define MEM_MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALIGN_TO(size, alignment) (((size) + (alignment)-1) & ~((alignment)-1))

// Taken from EASTL EA_PLATFORM_MIN_MALLOC_ALIGNMENT
#ifndef PLATFORM_MIN_MALLOC_ALIGNMENT
    #if defined(__APPLE__)
        #define PLATFORM_MIN_MALLOC_ALIGNMENT 16
    #elif defined(__ANDROID__) && defined(ARCH_ARM_FAMILY)
        #define PLATFORM_MIN_MALLOC_ALIGNMENT 8
    #elif defined(NX64) && defined(ARCH_ARM_FAMILY)
        #define PLATFORM_MIN_MALLOC_ALIGNMENT 8
    #elif defined(__ANDROID__) && defined(ARCH_X86_FAMILY)
        #define PLATFORM_MIN_MALLOC_ALIGNMENT 8
    #else
        #define PLATFORM_MIN_MALLOC_ALIGNMENT (PTR_SIZE * 2)
    #endif
#endif

#define MIN_ALLOC_ALIGNMENT PLATFORM_MIN_MALLOC_ALIGNMENT

// #define ENABLE_MEMORY_TRACKING
namespace aph::memory
{
#if defined(ENABLE_MEMORY_TRACKING)
    #include "mmgr.cpp"

void* malloc_internal(size_t size, const char* f, int l, const char* sf)
{
    return memalign_internal(MIN_ALLOC_ALIGNMENT, size, f, l, sf);
}

void* calloc_internal(size_t count, size_t size, const char* f, int l, const char* sf)
{
    return calloc_memalign_internal(count, MIN_ALLOC_ALIGNMENT, size, f, l, sf);
}

void* memalign_internal(size_t align, size_t size, const char* f, int l, const char* sf)
{
    void* pMemAlign = m_allocator(f, l, sf, m_alloc_malloc, align, size);

    return pMemAlign;
}

void* calloc_memalign_internal(size_t count, size_t align, size_t size, const char* f, int l, const char* sf)
{
    size = ALIGN_TO(size, align);

    void* pMemAlign = m_allocator(f, l, sf, m_alloc_calloc, align, size * count);

    return pMemAlign;
}

void* realloc_internal(void* ptr, size_t size, const char* f, int l, const char* sf)
{
    void* pRealloc = m_reallocator(f, l, sf, m_alloc_realloc, size, ptr);

    return pRealloc;
}

void free_internal(void* ptr, const char* f, int l, const char* sf)
{
    m_deallocator(f, l, sf, m_alloc_free, ptr);
}

#else

void* malloc_internal(size_t size, const char* f, int l, const char* sf)
{
    return std::malloc(size);
}

void* memalign_internal(size_t align, size_t size, const char* f, int l, const char* sf)
{
    return std::aligned_alloc(align, size);
}

void* calloc_internal(size_t count, size_t size, const char* f, int l, const char* sf)
{
    return std::calloc(count, size);
}

void* calloc_memalign(size_t count, size_t alignment, size_t size)
{
    size_t alignedArrayElementSize = ALIGN_TO(size, alignment);
    size_t totalBytes              = count * alignedArrayElementSize;

    void* ptr = aph_memalign(alignment, totalBytes);

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
#endif
}  // namespace aph::memory
