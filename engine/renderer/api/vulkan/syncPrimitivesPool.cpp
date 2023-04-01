#include "syncPrimitivesPool.h"

namespace vkl
{
VulkanSyncPrimitivesPool::VulkanSyncPrimitivesPool(VulkanDevice *device) : m_device(device)
{
}

VulkanSyncPrimitivesPool::~VulkanSyncPrimitivesPool()
{
    // Destroy all created fences.
    for(auto fence : m_allFences)
        vkDestroyFence(m_device->getHandle(), fence, nullptr);

    // Destroy all created semaphores.
    for(auto semaphore : m_allSemaphores)
        vkDestroySemaphore(m_device->getHandle(), semaphore, nullptr);
}

VkResult VulkanSyncPrimitivesPool::acquireFence(VkFence &fence, bool isSignaled)
{
    VkResult result = VK_SUCCESS;

    // See if there's a free fence available.
    m_fenceLock.Lock();
    if(!m_availableFences.empty())
    {
        fence = m_availableFences.front();
        m_availableFences.pop();
    }
    // Else create a new one.
    else
    {
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if(isSignaled)
        {
            createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        result = vkCreateFence(m_device->getHandle(), &createInfo, nullptr, &fence);
        if(result == VK_SUCCESS)
            m_allFences.emplace(fence);
    }
    m_fenceLock.Unlock();

    return result;
}

void VulkanSyncPrimitivesPool::ReleaseFence(VkFence fence)
{
    m_fenceLock.Lock();
    if(m_allFences.find(fence) != m_allFences.end())
    {
        vkResetFences(m_device->getHandle(), 1, &fence);
        m_availableFences.push(fence);
    }
    m_fenceLock.Unlock();
}

bool VulkanSyncPrimitivesPool::Exists(VkFence fence)
{
    m_fenceLock.Lock();
    auto result = (m_allFences.find(fence) != m_allFences.end());
    m_fenceLock.Unlock();
    return result;
}

VkResult VulkanSyncPrimitivesPool::acquireSemaphore(uint32_t semaphoreCount, VkSemaphore *pSemaphores)
{
    VkResult result = VK_SUCCESS;

    // See if there are free semaphores available.
    m_semaphoreLock.Lock();
    while(!m_availableSemaphores.empty())
    {
        *pSemaphores = m_availableSemaphores.front();
        m_availableSemaphores.pop();
        ++pSemaphores;
        if(--semaphoreCount == 0)
            break;
    }

    // Create any remaining required semaphores.
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result = vkCreateSemaphore(m_device->getHandle(), &createInfo, nullptr, &pSemaphores[i]);
        if(result != VK_SUCCESS)
            break;

        m_allSemaphores.emplace(pSemaphores[i]);
    }

    m_semaphoreLock.Unlock();
    return result;
}

void VulkanSyncPrimitivesPool::ReleaseSemaphores(uint32_t semaphoreCount, const VkSemaphore *pSemaphores)
{
    m_semaphoreLock.Lock();
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        if(m_allSemaphores.find(pSemaphores[i]) != m_allSemaphores.end())
        {
            m_availableSemaphores.push(pSemaphores[i]);
        }
    }
    m_semaphoreLock.Unlock();
}

bool VulkanSyncPrimitivesPool::Exists(VkSemaphore semaphore)
{
    m_semaphoreLock.Lock();
    auto result = (m_allSemaphores.find(semaphore) != m_allSemaphores.end());
    m_semaphoreLock.Unlock();
    return result;
}
}  // namespace vkl
