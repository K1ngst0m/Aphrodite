#include "syncPrimitive.h"
#include "device.h"

namespace aph::vk
{
Fence::Fence(Device* pDevice, HandleType handle)
    : ResourceHandle(handle)
    , m_pDevice(pDevice)
{
}
void Fence::reset()
{
    std::lock_guard<std::mutex> holder{ m_lock };
    if (getHandle() != VK_NULL_HANDLE)
    {
        m_pDevice->getHandle().resetFences({ getHandle() });
    }
}
SyncPrimitiveAllocator::SyncPrimitiveAllocator(Device* device)
    : m_pDevice(device)
{
}

SyncPrimitiveAllocator::~SyncPrimitiveAllocator()
{
    clear();
}

Result SyncPrimitiveAllocator::acquireFence(Fence** ppFence, bool isSignaled)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> lock{ m_fenceLock };
    auto& pFence = *ppFence;

    // See if there's a free fence available.
    if (!m_availableFences.empty())
    {
        auto vkFence = m_availableFences.front()->getHandle();
        pFence = m_fencePool.allocate(m_pDevice, vkFence);
        m_availableFences.pop();
    }
    // Else create a new one.
    else
    {
        ::vk::FenceCreateInfo createInfo = {};
        if (isSignaled)
        {
            createInfo.setFlags(::vk::FenceCreateFlagBits::eSignaled);
        }
        auto [result, vkFence] = m_pDevice->getHandle().createFence(createInfo, vk_allocator());

        if (result == ::vk::Result::eSuccess)
        {
            pFence = m_fencePool.allocate(m_pDevice, vkFence);
            m_allFences.emplace(pFence);
        }
        else
        {
            APH_ASSERT(false);
            return { Result::RuntimeError, "Failed to acquire fence." };
        }
    }

    return Result::Success;
}

Result SyncPrimitiveAllocator::releaseFence(Fence* pFence)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> lock{ m_fenceLock };

    if (m_allFences.contains(pFence))
    {
        auto result = m_pDevice->getHandle().resetFences({ pFence->getHandle() });
        if (result != ::vk::Result::eSuccess)
        {
            m_fenceLock.unlock();
            return { Result::RuntimeError, "Failed to reset fence." };
        }
        m_availableFences.push(pFence);
    }

    return Result::Success;
}

bool SyncPrimitiveAllocator::Exists(Fence* pFence)
{
    std::lock_guard<std::mutex> lock{ m_fenceLock };

    auto result = (m_allFences.find(pFence) != m_allFences.end());

    return result;
}

Result SyncPrimitiveAllocator::acquireSemaphore(uint32_t semaphoreCount, Semaphore** ppSemaphores)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> lock{ m_semaphoreLock };

    // See if there are free semaphores available.
    while (!m_availableSemaphores.empty())
    {
        auto& pSemaphore = *ppSemaphores;
        VkSemaphore vkSemaphore = m_availableSemaphores.front()->getHandle();
        pSemaphore = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
        m_availableSemaphores.pop();
        ++ppSemaphores;
        if (--semaphoreCount == 0)
        {
            break;
        }
    }

    // Create any remaining required semaphores.
    for (auto i = 0U; i < semaphoreCount; ++i)
    {
        ::vk::SemaphoreCreateInfo createInfo = {};
        auto [result, vkSemaphore] = m_pDevice->getHandle().createSemaphore(createInfo, vk_allocator());
        ppSemaphores[i] = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
        if (result != ::vk::Result::eSuccess)
        {
            return { Result::RuntimeError, "Failed to acquire semaphore." };
        }

        m_allSemaphores.emplace(ppSemaphores[i]);
    }

    return Result::Success;
}

Result SyncPrimitiveAllocator::ReleaseSemaphores(uint32_t semaphoreCount, Semaphore** ppSemaphores)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> lock{ m_semaphoreLock };
    for (auto i = 0U; i < semaphoreCount; ++i)
    {
        if (m_allSemaphores.contains(ppSemaphores[i]))
        {
            m_availableSemaphores.push(ppSemaphores[i]);
        }
    }
    return Result::Success;
}

bool SyncPrimitiveAllocator::Exists(Semaphore* semaphore)
{
    std::lock_guard<std::mutex> lock{ m_semaphoreLock };
    auto result = m_allSemaphores.contains(semaphore);
    return result;
}

bool Fence::wait(uint64_t timeout)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> holder{ m_lock };
    bool result;

    // Waiting for the same VkFence in parallel is not allowed, and there seems to be some shenanigans on Intel
    // when waiting for a timeline semaphore in parallel with same value as well.

    if (timeout == 0)
    {
        result = m_pDevice->getHandle().getFenceStatus(getHandle()) == ::vk::Result::eSuccess;
    }
    else
    {
        result = m_pDevice->waitForFence({ this }, true, timeout).success();
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
        std::lock_guard<std::mutex> lock{ m_fenceLock };
        for (auto* fence : m_allFences)
        {
            m_pDevice->getHandle().destroyFence(fence->getHandle(), vk_allocator());
        }
        m_allFences.clear();
    }

    // Destroy all created semaphores.
    {
        std::lock_guard<std::mutex> lock{ m_semaphoreLock };
        for (auto* semaphore : m_allSemaphores)
        {
            m_pDevice->getHandle().destroySemaphore(semaphore->getHandle(), vk_allocator());
        }
        m_allSemaphores.clear();
    }
}
} // namespace aph::vk
