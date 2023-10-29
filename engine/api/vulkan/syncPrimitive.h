#ifndef SYNCPRIMITIVESPOOL_H_
#define SYNCPRIMITIVESPOOL_H_

#include "vkUtils.h"

namespace aph::vk
{
class Device;

class Fence : public ResourceHandle<VkFence>
{
    friend class Device;
    friend class SyncPrimitivesPool;
    friend class ObjectPool<Fence>;

public:
    bool wait(uint64_t timeout = UINT64_MAX);

private:
    Fence(Device* pDevice, HandleType handle) : ResourceHandle(handle), m_pDevice(pDevice) {}
    ~Fence();

    Device*    m_pDevice      = {};
    bool       m_observedWait = {false};
    std::mutex m_lock         = {};
};

class Semaphore : public ResourceHandle<VkSemaphore>
{
    friend class Device;
    friend class SyncPrimitivesPool;
    friend class ObjectPool<Semaphore>;

public:
    bool isSignaled() const { return m_signaled; }

private:
    Semaphore(Device* pDevice, HandleType handle) : ResourceHandle(handle), m_pDevice(pDevice) {}

    bool    m_signaled = {false};
    Device* m_pDevice  = {};
};

class SyncPrimitivesPool
{
public:
    SyncPrimitivesPool(Device* device);

    ~SyncPrimitivesPool();

    void clear();

    VkResult acquireFence(Fence** fence, bool isSignaled = true);
    VkResult releaseFence(Fence* fence);

    VkResult acquireSemaphore(uint32_t semaphoreCount, Semaphore** ppSemaphores);
    VkResult ReleaseSemaphores(uint32_t semaphoreCount, Semaphore** ppSemaphores);

    bool Exists(Fence* fence);
    bool Exists(VkSemaphore semaphore);

private:
    Device*          m_pDevice      = {};
    VolkDeviceTable* m_pDeviceTable = {};

    std::set<VkFence>     m_allFences       = {};
    std::queue<VkFence>   m_availableFences = {};
    ObjectPool<Semaphore> m_semaphorePool   = {};

    std::set<VkSemaphore>   m_allSemaphores       = {};
    std::queue<VkSemaphore> m_availableSemaphores = {};
    ObjectPool<Fence>       m_fencePool           = {};

    std::mutex m_fenceLock     = {};
    std::mutex m_semaphoreLock = {};
};
}  // namespace aph::vk

#endif  // SYNCPRIMITIVESPOOL_H_
