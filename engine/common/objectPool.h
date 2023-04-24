#ifndef OBJECTPOOL_H_
#define OBJECTPOOL_H_

#include "alignedAlloc.h"

namespace aph
{
template <typename T>
class ObjectPool
{
public:
    template <typename... P>
    T* allocate(P&&... p)
    {
        if(vacants.empty())
        {
            unsigned num_objects = 64u << memory.size();
            T*       ptr = static_cast<T*>(memAlignAlloc(std::max<size_t>(64, alignof(T)), num_objects * sizeof(T)));
            if(!ptr) return nullptr;

            for(unsigned i = 0; i < num_objects; i++)
                vacants.push_back(&ptr[i]);

            memory.emplace_back(ptr);
        }

        T* ptr = vacants.back();
        vacants.pop_back();
        new(ptr) T(std::forward<P>(p)...);
        return ptr;
    }

    void free(T* ptr)
    {
        ptr->~T();
        vacants.push_back(ptr);
    }

    void clear()
    {
        vacants.clear();
        memory.clear();
    }

protected:
    std::vector<T*> vacants;

    struct MallocDeleter
    {
        void operator()(T* ptr) { memalign_free(ptr); }
    };

    std::vector<std::unique_ptr<T, MallocDeleter>> memory;
};
}  // namespace aph

#endif  // OBJECTPOOL_H_
