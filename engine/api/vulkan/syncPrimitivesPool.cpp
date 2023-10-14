#include "syncPrimitivesPool.h"
#include "device.h"

namespace aph::vk
{
SyncPrimitivesPool::SyncPrimitivesPool(Device* device) : m_device(device)
{
}

SyncPrimitivesPool::~SyncPrimitivesPool()
{
    // Destroy all created fences.
    for(auto* fence : m_allFences)
        vkDestroyFence(m_device->getHandle(), fence, vk::vkAllocator());

    // Destroy all created semaphores.
    for(auto* semaphore : m_allSemaphores)
        vkDestroySemaphore(m_device->getHandle(), semaphore, vk::vkAllocator());

    for(auto* semaphore : m_allTimelineSemahpores)
        vkDestroySemaphore(m_device->getHandle(), semaphore, vk::vkAllocator());
}

VkResult SyncPrimitivesPool::acquireFence(VkFence& fence, bool isSignaled)
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
        createInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if(isSignaled)
        {
            createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        result = vkCreateFence(m_device->getHandle(), &createInfo, vk::vkAllocator(), &fence);
        if(result == VK_SUCCESS)
            m_allFences.emplace(fence);
    }
    m_fenceLock.Unlock();

    return result;
}

VkResult SyncPrimitivesPool::releaseFence(VkFence fence)
{
    m_fenceLock.Lock();
    if(m_allFences.count(fence))
    {
        VkResult result = vkResetFences(m_device->getHandle(), 1, &fence);
        if(result != VK_SUCCESS)
        {
            m_fenceLock.Unlock();
            return result;
        }
        m_availableFences.push(fence);
    }
    m_fenceLock.Unlock();
    return VK_SUCCESS;
}

bool SyncPrimitivesPool::Exists(VkFence fence)
{
    m_fenceLock.Lock();
    auto result = (m_allFences.find(fence) != m_allFences.end());
    m_fenceLock.Unlock();
    return result;
}

VkResult SyncPrimitivesPool::acquireSemaphore(uint32_t semaphoreCount, VkSemaphore* pSemaphores)
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
        createInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result = vkCreateSemaphore(m_device->getHandle(), &createInfo, vk::vkAllocator(), &pSemaphores[i]);
        if(result != VK_SUCCESS)
            break;

        m_allSemaphores.emplace(pSemaphores[i]);
    }

    m_semaphoreLock.Unlock();
    return result;
}

VkResult SyncPrimitivesPool::ReleaseSemaphores(uint32_t semaphoreCount, const VkSemaphore* pSemaphores)
{
    m_semaphoreLock.Lock();
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        if(m_allSemaphores.count(pSemaphores[i]))
        {
            m_availableSemaphores.push(pSemaphores[i]);
        }
    }
    m_semaphoreLock.Unlock();
    return VK_SUCCESS;
}

bool SyncPrimitivesPool::Exists(VkSemaphore semaphore)
{
    m_semaphoreLock.Lock();
    auto result = (m_allSemaphores.find(semaphore) != m_allSemaphores.end());
    m_semaphoreLock.Unlock();
    return result;
}

VkResult SyncPrimitivesPool::acquireTimelineSemaphore(uint32_t semaphoreCount, VkSemaphore* pSemaphores, uint32_t init)
{
    VkResult result = VK_SUCCESS;

    // See if there are free semaphores available.
    m_timelineSemaphoreLock.Lock();
    while(!m_availableTimelineSemaphores.empty())
    {
        *pSemaphores = m_availableTimelineSemaphores.front();
        m_availableTimelineSemaphores.pop();
        ++pSemaphores;
        if(--semaphoreCount == 0)
            break;
    }

    // Create any remaining required semaphores.
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        VkSemaphoreTypeCreateInfo timelineCreateInfo{
            .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext         = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue  = init,
        };

        VkSemaphoreCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &timelineCreateInfo,
            .flags = 0,
        };
        result = vkCreateSemaphore(m_device->getHandle(), &createInfo, vk::vkAllocator(), &pSemaphores[i]);
        if(result != VK_SUCCESS)
        {
            break;
        }

        m_allTimelineSemahpores.emplace(pSemaphores[i]);
    }

    m_timelineSemaphoreLock.Unlock();
    return result;
}
}  // namespace aph::vk
