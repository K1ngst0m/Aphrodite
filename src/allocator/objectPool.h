#pragma once

#include "allocator/allocator.h"
#include "common/debug.h"
#include "common/hash.h"
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
        if (m_vacants.empty())
        {
            unsigned num_objects = 64u << m_memory.size();
            T* ptr = static_cast<T*>(memory::aph_memalign(std::max<size_t>(64, alignof(T)), num_objects * sizeof(T)));
            if (!ptr)
            {
                return nullptr;
            }

            for (unsigned i = 0; i < num_objects; i++)
            {
                m_vacants.push_back(&ptr[i]);
            }

            m_memory.emplace_back(ptr);
        }

        T* ptr = m_vacants.back();
        m_vacants.pop_back();
        new (ptr) T(std::forward<P>(p)...);
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
        void operator()(T* ptr)
        {
            memory::aph_free(ptr);
        }
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

template <typename BaseT>
class PolymorphicObjectPool
{
private:
    // Type-erased destructor function pointer
    using DestructorFn = void (*)(BaseT*);

    // Object metadata
    struct ObjectMetadata
    {
        DestructorFn destructor;
        size_t sizeInBytes; // Store actual size of the derived object
    };

    // Map to store destructors for each object
    HashMap<BaseT*, ObjectMetadata> m_metadata;

    // Type registry to store sizes - used to allocate chunks with the correct size
    struct TypeInfo
    {
        size_t size;
        size_t alignment;
    };

    HashMap<size_t, TypeInfo> m_typeRegistry;

    // Get a type hash without using RTTI
    template <typename T>
    static size_t getTypeHash()
    {
        // Use address of static function as type ID - safer than sizeof
        static char typeIdVar;
        return reinterpret_cast<size_t>(&typeIdVar);
    }

public:
    // Allocate a derived type using the base class pool
    template <typename DerivedT, typename... Args>
    DerivedT* allocate(Args&&... args)
    {
        static_assert(std::is_base_of<BaseT, DerivedT>::value, "DerivedT must inherit from BaseT");

        // Get or register type info for this derived type
        size_t typeHash = getTypeHash<DerivedT>();
        if (!m_typeRegistry.contains(typeHash))
        {
            m_typeRegistry[typeHash] = TypeInfo{sizeof(DerivedT), alignof(DerivedT)};
        }

        const TypeInfo& typeInfo = m_typeRegistry[typeHash];

        // Allocate properly sized and aligned memory
        void* memory = memory::aph_memalign(typeInfo.alignment, typeInfo.size);

        APH_ASSERT(memory);

        if (!memory)
        {
            return nullptr;
        }

        // Track this memory allocation
        m_allocations.emplace_back(memory);

        // Construct the derived object
        DerivedT* derivedPtr = new (memory) DerivedT(std::forward<Args>(args)...);
        BaseT* basePtr = static_cast<BaseT*>(derivedPtr);

        // Store metadata about this object
        m_metadata[basePtr] = {[](BaseT* ptr) { static_cast<DerivedT*>(ptr)->~DerivedT(); }, typeInfo.size};

        return derivedPtr;
    }

    void free(BaseT* ptr)
    {
        if (!ptr)
        {
            return;
        }

        if (auto it = m_metadata.find(ptr); it != m_metadata.end())
        {
            // Call the correct destructor
            it->second.destructor(ptr);

            // Find and remove from allocations list
            auto allocIt = std::find_if(m_allocations.begin(), m_allocations.end(),
                                        [ptr](const std::unique_ptr<void, MallocDeleter>& alloc)
                                        { return alloc.get() == static_cast<void*>(ptr); });

            if (allocIt != m_allocations.end())
            {
                // Move the allocation to the free list
                m_allocations.erase(allocIt);
            }

            // Remove metadata
            m_metadata.erase(it);
        }
    }

    void clear()
    {
        // Call destructors on any active objects
        for (auto& pair : m_metadata)
        {
            pair.second.destructor(pair.first);
        }

        m_metadata.clear();
        m_allocations.clear();
    }

    ~PolymorphicObjectPool()
    {
        clear();
    }

protected:
    struct MallocDeleter
    {
        void operator()(void* ptr)
        {
            memory::aph_free(ptr);
        }
    };

    // All allocated memory blocks
    SmallVector<std::unique_ptr<void, MallocDeleter>> m_allocations;
};

// Thread-safe version of the polymorphic pool
template <typename BaseT>
class ThreadSafePolymorphicObjectPool : private PolymorphicObjectPool<BaseT>
{
public:
    template <typename DerivedT, typename... Args>
    DerivedT* allocate(Args&&... args)
    {
        std::lock_guard<std::mutex> holder{m_lock};
        return PolymorphicObjectPool<BaseT>::template allocate<DerivedT>(std::forward<Args>(args)...);
    }

    void free(BaseT* ptr)
    {
        std::lock_guard<std::mutex> holder{m_lock};
        PolymorphicObjectPool<BaseT>::free(ptr);
    }

    void clear()
    {
        std::lock_guard<std::mutex> holder{m_lock};
        PolymorphicObjectPool<BaseT>::clear();
    }

private:
    std::mutex m_lock;
};
} // namespace aph
