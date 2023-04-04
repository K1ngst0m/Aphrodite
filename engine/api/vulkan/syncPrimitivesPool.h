#ifndef SYNCPRIMITIVESPOOL_H_
#define SYNCPRIMITIVESPOOL_H_

#include "common/spinlock.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;
class VulkanSyncPrimitivesPool
{
public:
    VulkanSyncPrimitivesPool(VulkanDevice *device);

    ~VulkanSyncPrimitivesPool();

    VkResult acquireFence(VkFence &fence, bool isSignaled = true);
    VkResult releaseFence(VkFence fence);
    bool Exists(VkFence fence);

    VkResult acquireSemaphore(uint32_t semaphoreCount, VkSemaphore *pSemaphores);
    VkResult ReleaseSemaphores(uint32_t semaphoreCount, const VkSemaphore *pSemaphores);
    bool Exists(VkSemaphore semaphore);

private:
    VulkanDevice *m_device = nullptr;
    std::set<VkFence> m_allFences;
    std::set<VkSemaphore> m_allSemaphores;
    std::queue<VkFence> m_availableFences;
    std::queue<VkSemaphore> m_availableSemaphores;
    SpinLock m_fenceLock, m_semaphoreLock;
};
}  // namespace vkl

#endif  // SYNCPRIMITIVESPOOL_H_
