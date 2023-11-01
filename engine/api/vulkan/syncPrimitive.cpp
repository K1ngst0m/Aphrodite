#include "syncPrimitive.h"
#include "device.h"

namespace aph::vk
{
SyncPrimitivesPool::SyncPrimitivesPool(Device* device) : m_pDevice(device), m_pDeviceTable(device->getDeviceTable())
{
}

SyncPrimitivesPool::~SyncPrimitivesPool()
{
}

VkResult SyncPrimitivesPool::acquireFence(Fence** ppFence, bool isSignaled)
{
    VkResult result = VK_SUCCESS;
    auto&    pFence = *ppFence;

    // See if there's a free fence available.
    {
        std::lock_guard<std::mutex> lock{m_fenceLock};

        if(!m_availableFences.empty())
        {
            auto vkFence = m_availableFences.front();
            pFence       = m_fencePool.allocate(m_pDevice, vkFence);
            m_availableFences.pop();
        }
        // Else create a new one.
        else
        {
            VkFenceCreateInfo createInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            if(isSignaled)
            {
                createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            }
            VkFence vkFence;
            result = m_pDeviceTable->vkCreateFence(m_pDevice->getHandle(), &createInfo, vk::vkAllocator(), &vkFence);
            if(result == VK_SUCCESS)
            {
                m_allFences.emplace(vkFence);
                pFence = m_fencePool.allocate(m_pDevice, vkFence);
            }
            else
            {
                APH_ASSERT(false);
            }
        }
    }

    return result;
}

VkResult SyncPrimitivesPool::releaseFence(Fence* pFence)
{
    std::lock_guard<std::mutex> lock{m_fenceLock};

    if(m_allFences.contains(pFence->getHandle()))
    {
        VkResult result = m_pDeviceTable->vkResetFences(m_pDevice->getHandle(), 1, &pFence->getHandle());
        if(result != VK_SUCCESS)
        {
            m_fenceLock.unlock();
            return result;
        }
        m_availableFences.push(pFence->getHandle());
    }

    return VK_SUCCESS;
}

bool SyncPrimitivesPool::Exists(Fence* pFence)
{
    std::lock_guard<std::mutex> lock{m_fenceLock};

    auto result = (m_allFences.find(pFence->getHandle()) != m_allFences.end());

    return result;
}

VkResult SyncPrimitivesPool::acquireSemaphore(uint32_t semaphoreCount, Semaphore** ppSemaphores)
{
    VkResult result = VK_SUCCESS;

    // See if there are free semaphores available.
    std::lock_guard<std::mutex> lock{m_semaphoreLock};
    while(!m_availableSemaphores.empty())
    {
        auto&       pSemaphore  = *ppSemaphores;
        VkSemaphore vkSemaphore = m_availableSemaphores.front();
        pSemaphore              = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
        m_availableSemaphores.pop();
        ++ppSemaphores;
        if(--semaphoreCount == 0)
        {
            break;
        }
    }

    // Create any remaining required semaphores.
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        VkSemaphore           vkSemaphore;
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result          = m_pDeviceTable->vkCreateSemaphore(m_pDevice->getHandle(), &createInfo, vk::vkAllocator(), &vkSemaphore);
        ppSemaphores[i] = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
        if(result != VK_SUCCESS)
        {
            break;
        }

        m_allSemaphores.emplace(vkSemaphore);
    }

    return result;
}

VkResult SyncPrimitivesPool::ReleaseSemaphores(uint32_t semaphoreCount, Semaphore** ppSemaphores)
{
    std::lock_guard<std::mutex> lock{m_semaphoreLock};
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        VkSemaphore vkSemaphore = ppSemaphores[i]->getHandle();
        if(m_allSemaphores.contains(vkSemaphore))
        {
            m_availableSemaphores.push(vkSemaphore);
        }
    }
    return VK_SUCCESS;
}

bool SyncPrimitivesPool::Exists(VkSemaphore semaphore)
{
    std::lock_guard<std::mutex> lock{m_semaphoreLock};
    auto result = m_allSemaphores.contains(semaphore);
    return result;
}

bool Fence::wait(uint64_t timeout)
{
    bool  result;
    auto* table = m_pDevice->getDeviceTable();

    // Waiting for the same VkFence in parallel is not allowed, and there seems to be some shenanigans on Intel
    // when waiting for a timeline semaphore in parallel with same value as well.
    std::lock_guard<std::mutex> holder{m_lock};

    if(m_observedWait)
    {
        return true;
    }

    if(timeout == 0)
    {
        result = table->vkGetFenceStatus(m_pDevice->getHandle(), getHandle()) == VK_SUCCESS;
    }
    else
    {
        result = table->vkWaitForFences(m_pDevice->getHandle(), 1, &getHandle(), VK_TRUE, timeout) == VK_SUCCESS;
    }

    if(result)
    {
        m_observedWait = true;
    }
    else
    {
        VK_LOG_ERR("Failed to wait for fence!");
    }

    return result;
}

Fence::~Fence()
{
    if(getHandle() != VK_NULL_HANDLE)
    {
        vkResetFences(m_pDevice->getHandle(), 1, &getHandle());
    }
}
void SyncPrimitivesPool::clear()
{
    // Destroy all created fences.
    for(auto* fence : m_allFences)
        m_pDeviceTable->vkDestroyFence(m_pDevice->getHandle(), fence, vk::vkAllocator());
    m_allFences.clear();

    // Destroy all created semaphores.
    for(auto* semaphore : m_allSemaphores)
        m_pDeviceTable->vkDestroySemaphore(m_pDevice->getHandle(), semaphore, vk::vkAllocator());
    m_allSemaphores.clear();
}
}  // namespace aph::vk
