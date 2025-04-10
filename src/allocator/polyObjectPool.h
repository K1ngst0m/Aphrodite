#pragma once

#include "allocator/objectPool.h"
#include "common/hash.h"

namespace aph
{
template <typename BaseT>
class PolymorphicObjectPool
{
public:
    PolymorphicObjectPool() = default;

    // Delete copy and move operations
    PolymorphicObjectPool(const PolymorphicObjectPool&)            = delete;
    PolymorphicObjectPool& operator=(const PolymorphicObjectPool&) = delete;
    PolymorphicObjectPool(PolymorphicObjectPool&&)                 = delete;
    PolymorphicObjectPool& operator=(PolymorphicObjectPool&&)      = delete;

    // Allocate a derived type using the base class pool
    template <typename DerivedT, typename... Args>
    DerivedT* allocate(Args&&... args);

    void free(BaseT* ptr);

    void clear();

    // Get current number of allocated objects (for debugging)
    size_t getAllocationCount() const;

    ~PolymorphicObjectPool();

private:
    // Type-erased destructor function pointer
    using DestructorFn = void (*)(void*);

    // Store both the destructor and the original memory pointer
    struct AllocationInfo
    {
        DestructorFn destructor;
        void* memory;
    };

    // Track allocations with their destructors
    HashMap<BaseT*, AllocationInfo> m_allocations;

#ifdef APH_DEBUG
    // Debug tracking for allocations
    HashMap<BaseT*, PoolDebugInfo> m_debugInfo;
#endif
};

template <typename BaseT>
inline PolymorphicObjectPool<BaseT>::~PolymorphicObjectPool()
{
    clear();
}

template <typename BaseT>
inline size_t PolymorphicObjectPool<BaseT>::getAllocationCount() const
{
    return m_allocations.size();
}

template <typename BaseT>
inline void PolymorphicObjectPool<BaseT>::clear()
{
    // Make a copy to avoid iterator invalidation
    auto allocationsCopy = m_allocations;

    // Clear existing allocations first
    m_allocations.clear();

#ifdef APH_DEBUG
    m_debugInfo.clear();
#endif

    // Free all objects
    for (const auto& [ptr, info] : allocationsCopy)
    {
        // Call the type-specific destructor
        info.destructor(ptr);

        // Free memory
        memory::aph_free(info.memory);
    }
}

template <typename BaseT>
inline void PolymorphicObjectPool<BaseT>::free(BaseT* ptr)
{
    if (!ptr)
    {
        return;
    }

    // Check if this object belongs to this pool
    auto it = m_allocations.find(ptr);
    APH_ASSERT(it != m_allocations.end() && "Attempting to free an object not allocated from this pool");

    if (it == m_allocations.end())
    {
        return;
    }

    // Store the memory pointer and destructor
    AllocationInfo info = it->second;

    // Remove from tracking
    m_allocations.erase(it);

#ifdef APH_DEBUG
    m_debugInfo.erase(ptr);
#endif

    // Call the type-specific destructor
    info.destructor(ptr);

    // Free the memory
    memory::aph_free(info.memory);
}

template <typename BaseT>
template <typename DerivedT, typename... Args>
inline DerivedT* PolymorphicObjectPool<BaseT>::allocate(Args&&... args)
{
    static_assert(std::is_base_of<BaseT, DerivedT>::value, "DerivedT must inherit from BaseT");

    // Allocate memory for the derived type
    void* memory = memory::aph_memalign(alignof(DerivedT), sizeof(DerivedT));
    APH_ASSERT(memory && "Failed to allocate memory for polymorphic object");

    if (!memory)
    {
        return nullptr;
    }

    // Construct the derived object
    DerivedT* derivedPtr = new (memory) DerivedT(std::forward<Args>(args)...);
    BaseT* basePtr       = static_cast<BaseT*>(derivedPtr);

    // Store the allocation with its type-specific deleter
    m_allocations[basePtr] = {[](void* ptr) { static_cast<DerivedT*>(static_cast<BaseT*>(ptr))->~DerivedT(); }, memory};

#ifdef APH_DEBUG
    // Store allocation info for debugging
    PoolDebugInfo info{};
    std::source_location loc = std::source_location::current();
    info.file                = loc.file_name();
    info.line                = static_cast<int>(loc.line());
    info.function            = loc.function_name();
    m_debugInfo[basePtr]     = info;
#endif

    return derivedPtr;
}
} // namespace aph

namespace aph
{
// Concurrent node for polymorphic objects
template <typename BaseT>
struct ConcurrentPolymorphicNode
{
    BaseT* ptr;
    void* memory;
    void (*destructor)(void*);
    std::atomic<ConcurrentPolymorphicNode*> next;

    ConcurrentPolymorphicNode(BaseT* p, void* mem, void (*dest)(void*))
        : ptr(p)
        , memory(mem)
        , destructor(dest)
        , next(nullptr)
    {
    }
};

// Thread-safe version of the polymorphic pool
template <typename BaseT>
class ThreadSafePolymorphicObjectPool
{
public:
    ThreadSafePolymorphicObjectPool();

    // Delete copy and move operations
    ThreadSafePolymorphicObjectPool(const ThreadSafePolymorphicObjectPool&)            = delete;
    ThreadSafePolymorphicObjectPool& operator=(const ThreadSafePolymorphicObjectPool&) = delete;
    ThreadSafePolymorphicObjectPool(ThreadSafePolymorphicObjectPool&&)                 = delete;
    ThreadSafePolymorphicObjectPool& operator=(ThreadSafePolymorphicObjectPool&&)      = delete;

    template <typename DerivedT, typename... Args>
    DerivedT* allocate(Args&&... args);

    void free(BaseT* ptr);

    void clear();

    // Get current number of allocated objects (for debugging)
    size_t getAllocationCount() const;

    ~ThreadSafePolymorphicObjectPool();

private:
    // Head of our linked list
    std::atomic<ConcurrentPolymorphicNode<BaseT>*> m_head;

    // Count of active allocations
    std::atomic<size_t> m_activeCount;
};

template <typename BaseT>
inline ThreadSafePolymorphicObjectPool<BaseT>::~ThreadSafePolymorphicObjectPool()
{
    clear();
}

template <typename BaseT>
inline size_t ThreadSafePolymorphicObjectPool<BaseT>::getAllocationCount() const
{
    return m_activeCount.load(std::memory_order_relaxed);
}

template <typename BaseT>
inline void ThreadSafePolymorphicObjectPool<BaseT>::clear()
{
    // Use a linked list traversal to clear everything
    ConcurrentPolymorphicNode<BaseT>* current = m_head.exchange(nullptr, std::memory_order_acquire);

    // Create a new sentinel node for future use
    auto* sentinel = new ConcurrentPolymorphicNode<BaseT>(nullptr, nullptr, nullptr);
    sentinel->next.store(nullptr, std::memory_order_relaxed);
    m_head.store(sentinel, std::memory_order_release);

    // Reset count
    m_activeCount.store(0, std::memory_order_relaxed);

    // Free all nodes and objects
    while (current)
    {
        ConcurrentPolymorphicNode<BaseT>* next = current->next.load(std::memory_order_relaxed);

        // Free the object if it exists (skip sentinel nodes)
        if (current->ptr && current->memory)
        {
            current->destructor(current->ptr);
            memory::aph_free(current->memory);
        }

        // Free the node
        delete current;
        current = next;
    }
}

template <typename BaseT>
inline void ThreadSafePolymorphicObjectPool<BaseT>::free(BaseT* ptr)
{
    if (!ptr)
    {
        return;
    }

    // Find and remove the node from our linked list
    ConcurrentPolymorphicNode<BaseT>* prev = m_head.load(std::memory_order_acquire);
    if (!prev)
    {
        // Empty list
        APH_ASSERT(false && "Attempting to free an object not allocated from this pool");
        return;
    }

    // Special case for head node
    if (prev != nullptr && prev->ptr == ptr)
    {
        // Try to remove the head
        if (m_head.compare_exchange_strong(prev, prev->next.load(std::memory_order_relaxed), std::memory_order_release,
                                           std::memory_order_relaxed))
        {
            // Decrement count
            m_activeCount.fetch_sub(1, std::memory_order_relaxed);

            // Free the object and node
            prev->destructor(ptr);
            memory::aph_free(prev->memory);
            delete prev;
            return;
        }
    }

    // Traverse the list to find the node
    ConcurrentPolymorphicNode<BaseT>* current = prev ? prev->next.load(std::memory_order_relaxed) : nullptr;
    while (current)
    {
        if (current->ptr == ptr)
        {
            // Found it, try to remove it
            ConcurrentPolymorphicNode<BaseT>* nextNode = current->next.load(std::memory_order_relaxed);
            if (prev->next.compare_exchange_strong(current, nextNode, std::memory_order_release,
                                                   std::memory_order_relaxed))
            {
                // Decrement count
                m_activeCount.fetch_sub(1, std::memory_order_relaxed);

                // Free the object and node
                current->destructor(ptr);
                memory::aph_free(current->memory);
                delete current;
                return;
            }
            // Someone else changed the list, retry from the beginning
            return free(ptr);
        }
        prev    = current;
        current = current->next.load(std::memory_order_relaxed);
    }

    // Not found
    APH_ASSERT(false && "Attempting to free an object not allocated from this pool");
}

template <typename BaseT>
template <typename DerivedT, typename... Args>
inline DerivedT* ThreadSafePolymorphicObjectPool<BaseT>::allocate(Args&&... args)
{
    static_assert(std::is_base_of<BaseT, DerivedT>::value, "DerivedT must inherit from BaseT");

    // Allocate memory for the derived type
    void* memory = memory::aph_memalign(alignof(DerivedT), sizeof(DerivedT));
    APH_ASSERT(memory && "Failed to allocate memory for polymorphic object");

    if (!memory)
    {
        return nullptr;
    }

    // Construct the derived object
    DerivedT* derivedPtr = new (memory) DerivedT(std::forward<Args>(args)...);
    BaseT* basePtr       = static_cast<BaseT*>(derivedPtr);

    // Create destructor function
    auto destructor = [](void* ptr) { static_cast<DerivedT*>(static_cast<BaseT*>(ptr))->~DerivedT(); };

    // Create and add a new node to the list
    auto* newNode = new ConcurrentPolymorphicNode<BaseT>(basePtr, memory, destructor);

    // Add to the list using a lock-free approach
    ConcurrentPolymorphicNode<BaseT>* oldHead = m_head.load(std::memory_order_relaxed);
    do
    {
        newNode->next.store(oldHead, std::memory_order_relaxed);
    } while (!m_head.compare_exchange_weak(oldHead, newNode, std::memory_order_release, std::memory_order_relaxed));

    // Increment count
    m_activeCount.fetch_add(1, std::memory_order_relaxed);

    return derivedPtr;
}

template <typename BaseT>
inline ThreadSafePolymorphicObjectPool<BaseT>::ThreadSafePolymorphicObjectPool()
    : m_head(nullptr)
    , m_activeCount(0)
{
    // Initialize an empty sentinel node
    auto* sentinel = new ConcurrentPolymorphicNode<BaseT>(nullptr, nullptr, nullptr);
    sentinel->next.store(nullptr, std::memory_order_relaxed);
    m_head.store(sentinel, std::memory_order_relaxed);
}

} // namespace aph
