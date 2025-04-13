#pragma once

#include "allocator/objectPool.h"
#include "common/hash.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
class Fence : public ResourceHandle<::vk::Fence>
{
    friend class Device;
    friend class SyncPrimitiveAllocator;
    friend class ThreadSafeObjectPool<Fence>;

public:
    auto wait(uint64_t timeout = UINT64_MAX) -> bool;
    auto reset() -> void;

private:
    Fence(Device* pDevice, HandleType handle);
    ~Fence();

    Device* m_pDevice = {};
    std::mutex m_lock = {};
};

class Semaphore : public ResourceHandle<::vk::Semaphore>
{
    friend class Device;
    friend class SyncPrimitiveAllocator;
    friend class ThreadSafeObjectPool<Semaphore>;

public:
    auto isSignaled() const -> bool
    {
        return m_signaled;
    }

private:
    Semaphore(Device* pDevice, HandleType handle)
        : ResourceHandle(handle)
        , m_pDevice(pDevice)
    {
    }

    bool m_signaled   = {false};
    Device* m_pDevice = {};
};

class SyncPrimitiveAllocator
{
public:
    SyncPrimitiveAllocator(Device* device);

    ~SyncPrimitiveAllocator();

    auto clear() -> void;

    auto acquireFence(Fence** fence, bool isSignaled = true) -> Result;
    auto releaseFence(Fence* fence) -> Result;

    auto acquireSemaphore(uint32_t semaphoreCount, Semaphore** ppSemaphores) -> Result;
    auto ReleaseSemaphores(uint32_t semaphoreCount, Semaphore** ppSemaphores) -> Result;

    auto Exists(Fence* fence) -> bool;
    auto Exists(Semaphore* semaphore) -> bool;

private:
    Device* m_pDevice = {};

    HashSet<Fence*> m_allFences                     = {};
    std::queue<Fence*> m_availableFences            = {};
    ThreadSafeObjectPool<Semaphore> m_semaphorePool = {};

    HashSet<Semaphore*> m_allSemaphores          = {};
    std::queue<Semaphore*> m_availableSemaphores = {};
    ThreadSafeObjectPool<Fence> m_fencePool      = {};

    std::mutex m_fenceLock     = {};
    std::mutex m_semaphoreLock = {};
};
} // namespace aph::vk
