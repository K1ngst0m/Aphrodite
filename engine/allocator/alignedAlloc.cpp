#include "alignedAlloc.h"

namespace aph
{
void* memAlignAlloc(size_t alignment, size_t size)
{
#if defined(_WIN32)
    return _aligned_malloc(size, boundary);
#elif defined(_ISOC11_SOURCE)
    return aligned_alloc(alignment, (size + alignment - 1) & ~(alignment - 1));
#elif(_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600)
    void* ptr = nullptr;
    if(posix_memalign(&ptr, boundary, size) < 0)
        return nullptr;
    return ptr;
#else
    // Align stuff ourselves. Kinda ugly, but will work anywhere.
    void**    place;
    uintptr_t addr = 0;
    void*     ptr  = malloc(boundary + size + sizeof(uintptr_t));

    if(ptr == nullptr)
        return nullptr;

    addr      = ((uintptr_t)ptr + sizeof(uintptr_t) + boundary) & ~(boundary - 1);
    place     = (void**)addr;
    place[-1] = ptr;

    return (void*)addr;
#endif
}

void* memAlignCalloc(size_t boundary, size_t size)
{
    void* ret = memAlignAlloc(boundary, size);
    if(ret)
        memset(ret, 0, size);
    return ret;
}

void memAlignFree(void* ptr)
{
#if defined(_WIN32)
    _aligned_free(ptr);
#elif !defined(_ISOC11_SOURCE) && !((_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600))
    if(ptr != nullptr)
    {
        void** p = (void**)ptr;
        free(p[-1]);
    }
#else
    free(ptr);
#endif
}
}  // namespace aph
