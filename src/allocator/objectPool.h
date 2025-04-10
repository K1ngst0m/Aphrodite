#pragma once

#include "allocator/allocator.h"
#include "common/debug.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include <atomic>
#include <shared_mutex>

namespace aph
{
// Forward declaration for debugging
struct PoolDebugInfo
{
    const char* file;
    int line;
    const char* function;
};

template <typename T>
class ObjectPool
{
public:
    ObjectPool() = default;

    // Delete copy and move operations
    ObjectPool(const ObjectPool&)            = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&)                 = delete;
    ObjectPool& operator=(ObjectPool&&)      = delete;

    template <typename... P>
    T* allocate(P&&... p);

    void free(T* ptr);

    void clear();

    // Get current number of allocated objects (for debugging)
    size_t getAllocationCount() const;

    ~ObjectPool();

private:
    // Track all allocated objects
    HashSet<T*> m_allocations;

#ifdef APH_DEBUG
    // Debug tracking for allocations (file/line where allocated)
    HashMap<T*, PoolDebugInfo> m_debugInfo;
#endif
};

template <typename T>
inline ObjectPool<T>::~ObjectPool()
{
    clear();
}

template <typename T>
inline size_t ObjectPool<T>::getAllocationCount() const
{
    return m_allocations.size();
}

template <typename T>
inline void ObjectPool<T>::clear()
{
    // Make a copy of allocations to avoid iterator invalidation
    auto allocationsCopy = m_allocations;

    // Free all objects
    for (T* ptr : allocationsCopy)
    {
        // Call destructor
        ptr->~T();

        // Free memory
        memory::aph_free(ptr);
    }

    // Clear tracking
    m_allocations.clear();

#ifdef APH_DEBUG
    m_debugInfo.clear();
#endif
}

template <typename T>
inline void ObjectPool<T>::free(T* ptr)
{
    if (!ptr)
    {
        return;
    }

    // Check if this object belongs to this pool
    APH_ASSERT(m_allocations.contains(ptr) && "Attempting to free an object not allocated from this pool");

    if (!m_allocations.contains(ptr))
    {
        return;
    }

    // Remove from allocation tracking
    m_allocations.erase(ptr);

#ifdef APH_DEBUG
    m_debugInfo.erase(ptr);
#endif

    // Call destructor and free memory
    ptr->~T();
    memory::aph_free(ptr);
}

template <typename T>
template <typename... P>
inline T* ObjectPool<T>::allocate(P&&... p)
{
    // Allocate memory for the object
    void* memory = memory::aph_memalign(alignof(T), sizeof(T));
    APH_ASSERT(memory && "Failed to allocate memory");

    if (!memory)
    {
        return nullptr;
    }

    // Construct the object
    T* object = new (memory) T(std::forward<P>(p)...);

    // Track the allocation
    m_allocations.insert(object);

#ifdef APH_DEBUG
    // Store allocation info for debugging
    PoolDebugInfo info{};
    std::source_location loc = std::source_location::current();
    info.file                = loc.file_name();
    info.line                = static_cast<int>(loc.line());
    info.function            = loc.function_name();
    m_debugInfo[object]      = info;
#endif

    return object;
}
} // namespace aph

namespace aph
{
// Node for concurrent operations
template <typename T>
struct ConcurrentNode
{
    T* value;
    std::atomic<ConcurrentNode*> next;

    explicit ConcurrentNode(T* val)
        : value(val)
        , next(nullptr)
    {
    }
};

template <typename T>
class ThreadSafeObjectPool
{
public:
    ThreadSafeObjectPool();

    // Delete copy and move operations
    ThreadSafeObjectPool(const ThreadSafeObjectPool&)            = delete;
    ThreadSafeObjectPool& operator=(const ThreadSafeObjectPool&) = delete;
    ThreadSafeObjectPool(ThreadSafeObjectPool&&)                 = delete;
    ThreadSafeObjectPool& operator=(ThreadSafeObjectPool&&)      = delete;

    template <typename... P>
    T* allocate(P&&... p);

    void free(T* ptr);

    void clear();

    // Get current number of allocated objects (for debugging)
    size_t getAllocationCount() const;

    ~ThreadSafeObjectPool();

private:
    // Head of our linked list
    std::atomic<ConcurrentNode<T>*> m_head;

    // Count of active allocations
    std::atomic<size_t> m_activeCount;
};

template <typename T>
inline ThreadSafeObjectPool<T>::~ThreadSafeObjectPool()
{
    clear();
}

template <typename T>
inline size_t ThreadSafeObjectPool<T>::getAllocationCount() const
{
    return m_activeCount.load(std::memory_order_relaxed);
}

template <typename T>
inline void ThreadSafeObjectPool<T>::clear()
{
    // Use a linked list traversal to clear everything
    ConcurrentNode<T>* current = m_head.exchange(nullptr, std::memory_order_acquire);

    // Create a new sentinel node for future use
    auto* sentinel = new ConcurrentNode<T>(nullptr);
    sentinel->next.store(nullptr, std::memory_order_relaxed);
    m_head.store(sentinel, std::memory_order_release);

    // Reset count
    m_activeCount.store(0, std::memory_order_relaxed);

    // Free all nodes and objects
    while (current)
    {
        ConcurrentNode<T>* next = current->next.load(std::memory_order_relaxed);

        // Free the object if it exists (skip sentinel nodes)
        if (current->value)
        {
            current->value->~T();
            memory::aph_free(current->value);
        }

        // Free the node
        delete current;
        current = next;
    }
}

template <typename T>
inline void ThreadSafeObjectPool<T>::free(T* ptr)
{
    if (!ptr)
    {
        return;
    }

    // Find and remove the node from our linked list
    ConcurrentNode<T>* prev = m_head.load(std::memory_order_acquire);
    if (!prev)
    {
        // Empty list
        APH_ASSERT(false && "Attempting to free an object not allocated from this pool");
        return;
    }

    // Special case for head node
    if (prev != nullptr && prev->value == ptr)
    {
        // Try to remove the head
        if (m_head.compare_exchange_strong(prev, prev->next.load(std::memory_order_relaxed), std::memory_order_release,
                                           std::memory_order_relaxed))
        {
            // Decrement count
            m_activeCount.fetch_sub(1, std::memory_order_relaxed);

            // Free the object and node
            ptr->~T();
            memory::aph_free(ptr);
            delete prev;
            return;
        }
    }

    // Traverse the list to find the node
    ConcurrentNode<T>* current = prev ? prev->next.load(std::memory_order_relaxed) : nullptr;
    while (current)
    {
        if (current->value == ptr)
        {
            // Found it, try to remove it
            ConcurrentNode<T>* nextNode = current->next.load(std::memory_order_relaxed);
            if (prev->next.compare_exchange_strong(current, nextNode, std::memory_order_release,
                                                   std::memory_order_relaxed))
            {
                // Decrement count
                m_activeCount.fetch_sub(1, std::memory_order_relaxed);

                // Free the object and node
                ptr->~T();
                memory::aph_free(ptr);
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

template <typename T>
template <typename... P>
inline T* ThreadSafeObjectPool<T>::allocate(P&&... p)
{
    // Allocate memory for the object
    void* memory = memory::aph_memalign(alignof(T), sizeof(T));
    APH_ASSERT(memory && "Failed to allocate memory");

    if (!memory)
    {
        return nullptr;
    }

    // Construct the object
    T* object = new (memory) T(std::forward<P>(p)...);

    // Create a new node
    auto* newNode = new ConcurrentNode<T>(object);

    // Add to the list using a lock-free approach
    ConcurrentNode<T>* oldHead = m_head.load(std::memory_order_relaxed);
    do
    {
        newNode->next.store(oldHead, std::memory_order_relaxed);
    } while (!m_head.compare_exchange_weak(oldHead, newNode, std::memory_order_release, std::memory_order_relaxed));

    // Increment active count
    m_activeCount.fetch_add(1, std::memory_order_relaxed);

    return object;
}

template <typename T>
inline ThreadSafeObjectPool<T>::ThreadSafeObjectPool()
    : m_head(nullptr)
    , m_activeCount(0)
{
    // Initialize an empty sentinel node
    auto* sentinel = new ConcurrentNode<T>(nullptr);
    sentinel->next.store(nullptr, std::memory_order_relaxed);
    m_head.store(sentinel, std::memory_order_relaxed);
}

} // namespace aph
