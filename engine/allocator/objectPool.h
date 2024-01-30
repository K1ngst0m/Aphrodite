#ifndef OBJECTPOOL_H_
#define OBJECTPOOL_H_

#include "alignedAlloc.h"
#include "common/smallVector.h"

namespace aph
{
template <typename T>
class ObjectPool
{
public:
    template <typename... P>
    T* allocate(P&&... p)
    {
        if(m_vacants.empty())
        {
            unsigned num_objects = 64u << m_memory.size();
            T*       ptr = static_cast<T*>(memAlignAlloc(std::max<size_t>(64, alignof(T)), num_objects * sizeof(T)));
            if(!ptr)
                return nullptr;

            for(unsigned i = 0; i < num_objects; i++)
                m_vacants.push_back(&ptr[i]);

            m_memory.emplace_back(ptr);
        }

        T* ptr = m_vacants.back();
        m_vacants.pop_back();
        new(ptr) T(std::forward<P>(p)...);
        return ptr;
    }

    void free(T* ptr)
    {
        ptr->~T();
        m_vacants.push_back(ptr);
    }

    void clear()
    {
        m_vacants.clear();
        m_memory.clear();
    }

protected:
    SmallVector<T*> m_vacants;

    struct MallocDeleter
    {
        void operator()(T* ptr) { memAlignFree(ptr); }
    };

    SmallVector<std::unique_ptr<T, MallocDeleter>> m_memory;
};

template <typename T>
class ThreadSafeObjectPool : private ObjectPool<T>
{
public:
    template <typename... P>
    T* allocate(P&&... p)
    {
        std::lock_guard<std::mutex> holder{m_lock};
        return ObjectPool<T>::allocate(std::forward<P>(p)...);
    }

    void free(T* ptr)
    {
        // TODO only lock vector push operation
        std::lock_guard<std::mutex> holder{m_lock};
        ObjectPool<T>::free(ptr);
    }

    void clear()
    {
        std::lock_guard<std::mutex> holder{m_lock};
        ObjectPool<T>::clear();
    }

private:
    std::mutex m_lock;
};
}  // namespace aph

#endif  // OBJECTPOOL_H_
