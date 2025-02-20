#include "syncPrimitive.h"
#include "device.h"

namespace aph::vk
{
Fence::Fence(Device* pDevice, HandleType handle) : ResourceHandle(handle), m_pDevice(pDevice)
{
}
void Fence::reset()
{
    std::lock_guard<std::mutex> holder{m_lock};
    if(getHandle() != VK_NULL_HANDLE)
    {
        m_pDevice->getDeviceTable()->vkResetFences(m_pDevice->getHandle(), 1, &getHandle());
    }
}
SyncPrimitiveAllocator::SyncPrimitiveAllocator(Device* device) :
    m_pDevice(device),
    m_pDeviceTable(device->getDeviceTable())
{
}

SyncPrimitiveAllocator::~SyncPrimitiveAllocator()
{
    clear();
}

VkResult SyncPrimitiveAllocator::acquireFence(Fence** ppFence, bool isSignaled)
{
    std::lock_guard<std::mutex> lock{m_fenceLock};
    VkResult result = VK_SUCCESS;
    auto&    pFence = *ppFence;

    // See if there's a free fence available.
    if(!m_availableFences.empty())
    {
        auto vkFence = m_availableFences.front()->getHandle();
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
            pFence = m_fencePool.allocate(m_pDevice, vkFence);
            m_allFences.emplace(pFence);
        }
        else
        {
            APH_ASSERT(false);
        }
    }

    return result;
}

VkResult SyncPrimitiveAllocator::releaseFence(Fence* pFence)
{
    std::lock_guard<std::mutex> lock{m_fenceLock};

    if(m_allFences.contains(pFence))
    {
        VkResult result = m_pDeviceTable->vkResetFences(m_pDevice->getHandle(), 1, &pFence->getHandle());
        if(result != VK_SUCCESS)
        {
            m_fenceLock.unlock();
            return result;
        }
        m_availableFences.push(pFence);
    }

    return VK_SUCCESS;
}

bool SyncPrimitiveAllocator::Exists(Fence* pFence)
{
    std::lock_guard<std::mutex> lock{m_fenceLock};

    auto result = (m_allFences.find(pFence) != m_allFences.end());

    return result;
}

VkResult SyncPrimitiveAllocator::acquireSemaphore(uint32_t semaphoreCount, Semaphore** ppSemaphores)
{
    std::lock_guard<std::mutex> lock{m_semaphoreLock};
    VkResult result = VK_SUCCESS;

    // See if there are free semaphores available.
    while(!m_availableSemaphores.empty())
    {
        auto&       pSemaphore  = *ppSemaphores;
        VkSemaphore vkSemaphore = m_availableSemaphores.front()->getHandle();
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
        result =
            m_pDeviceTable->vkCreateSemaphore(m_pDevice->getHandle(), &createInfo, vk::vkAllocator(), &vkSemaphore);
        ppSemaphores[i] = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
        if(result != VK_SUCCESS)
        {
            break;
        }

        m_allSemaphores.emplace(ppSemaphores[i]);
    }

    return result;
}

VkResult SyncPrimitiveAllocator::ReleaseSemaphores(uint32_t semaphoreCount, Semaphore** ppSemaphores)
{
    std::lock_guard<std::mutex> lock{m_semaphoreLock};
    for(auto i = 0U; i < semaphoreCount; ++i)
    {
        if(m_allSemaphores.contains(ppSemaphores[i]))
        {
            m_availableSemaphores.push(ppSemaphores[i]);
        }
    }
    return VK_SUCCESS;
}

bool SyncPrimitiveAllocator::Exists(Semaphore* semaphore)
{
    std::lock_guard<std::mutex> lock{m_semaphoreLock};
    auto                        result = m_allSemaphores.contains(semaphore);
    return result;
}

bool Fence::wait(uint64_t timeout)
{
    std::lock_guard<std::mutex> holder{m_lock};
    bool  result;
    auto* table = m_pDevice->getDeviceTable();

    // Waiting for the same VkFence in parallel is not allowed, and there seems to be some shenanigans on Intel
    // when waiting for a timeline semaphore in parallel with same value as well.

    if(timeout == 0)
    {
        result = table->vkGetFenceStatus(m_pDevice->getHandle(), getHandle()) == VK_SUCCESS;
    }
    else
    {
        result = m_pDevice->waitForFence({this}, true, timeout).success();
    }

    // if(!result)
    // {
    //     VK_LOG_ERR("Failed to wait for fence!");
    // }

    return result;
}

Fence::~Fence()
{
    reset();
}
void SyncPrimitiveAllocator::clear()
{
    // Destroy all created fences.
    {
        std::lock_guard<std::mutex> lock{m_fenceLock};
        for(auto* fence : m_allFences)
        {
            m_pDeviceTable->vkDestroyFence(m_pDevice->getHandle(), fence->getHandle(), vk::vkAllocator());
        }
        m_allFences.clear();
    }

    // Destroy all created semaphores.
    {
        std::lock_guard<std::mutex> lock{m_semaphoreLock};
        for(auto* semaphore : m_allSemaphores)
        {
            m_pDeviceTable->vkDestroySemaphore(m_pDevice->getHandle(), semaphore->getHandle(), vk::vkAllocator());
        }
        m_allSemaphores.clear();
    }
}
}  // namespace aph::vk
