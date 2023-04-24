#ifndef ALIGNED_ALLOC_H_
#define ALIGNED_ALLOC_H_

#include "common.h"

namespace aph
{
void* memAlignAlloc(size_t boundary, size_t size);
void* memAlignCalloc(size_t boundary, size_t size);
void  memAlignFree(void* ptr);

struct AlignedDeleter
{
    void operator()(void* ptr) { memAlignFree(ptr); }
};

template <typename T>
struct AlignedAllocation
{
    static void* operator new(size_t size)
    {
        void* ret = ::aph::memAlignAlloc(alignof(T), size);
        if(!ret) throw std::bad_alloc();
        return ret;
    }

    static void* operator new[](size_t size)
    {
        void* ret = ::aph::memAlignAlloc(alignof(T), size);
        if(!ret) throw std::bad_alloc();
        return ret;
    }

    static void operator delete(void* ptr) { return ::aph::memAlignFree(ptr); }
    static void operator delete[](void* ptr) { return ::aph::memAlignFree(ptr); }
};

}  // namespace aph

#endif  // ALIGNED_ALLOC_H_
