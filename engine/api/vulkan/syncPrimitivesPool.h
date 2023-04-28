#ifndef SYNCPRIMITIVESPOOL_H_
#define SYNCPRIMITIVESPOOL_H_

#include "common/spinlock.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class SyncPrimitivesPool
{
public:
    SyncPrimitivesPool(Device* device);

    ~SyncPrimitivesPool();

    VkResult acquireFence(VkFence& fence, bool isSignaled = true);
    VkResult releaseFence(VkFence fence);
    bool     Exists(VkFence fence);

    VkResult acquireSemaphore(uint32_t semaphoreCount, VkSemaphore* pSemaphores);
    VkResult ReleaseSemaphores(uint32_t semaphoreCount, const VkSemaphore* pSemaphores);
    bool     Exists(VkSemaphore semaphore);

private:
    Device*                 m_device              = {};
    std::set<VkFence>       m_allFences           = {};
    std::set<VkSemaphore>   m_allSemaphores       = {};
    std::queue<VkFence>     m_availableFences     = {};
    std::queue<VkSemaphore> m_availableSemaphores = {};
    SpinLock                m_fenceLock = {}, m_semaphoreLock = {};
};
}  // namespace aph::vk

#endif  // SYNCPRIMITIVESPOOL_H_
