#ifndef ALIGNED_ALLOC_H_
#define ALIGNED_ALLOC_H_

#include "common.h"

namespace aph
{
void* memalignAlloc(size_t boundary, size_t size);
void* memalignCalloc(size_t boundary, size_t size);
void  memalignFree(void* ptr);

struct AlignedDeleter
{
    void operator()(void* ptr) { memalignFree(ptr); }
};

template <typename T>
struct AlignedAllocation
{
    static void* operator new(size_t size)
    {
        void* ret = ::aph::memalignAlloc(alignof(T), size);
        if(!ret) throw std::bad_alloc();
        return ret;
    }

    static void* operator new[](size_t size)
    {
        void* ret = ::aph::memalignAlloc(alignof(T), size);
        if(!ret) throw std::bad_alloc();
        return ret;
    }

    static void operator delete(void* ptr) { return ::aph::memalignFree(ptr); }
    static void operator delete[](void* ptr) { return ::aph::memalignFree(ptr); }
};

}  // namespace aph

#endif  // ALIGNED_ALLOC_H_
