#include "allocator.h"
#include <cstdlib>
#include <cstring>
#include <malloc.h>

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
