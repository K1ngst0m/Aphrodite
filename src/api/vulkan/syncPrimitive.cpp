#include "syncPrimitive.h"
#include "device.h"

namespace aph::vk
{
Fence::Fence(Device* pDevice, HandleType handle)
    : ResourceHandle(handle)
    , m_pDevice(pDevice)
{
}

auto Fence::reset() -> void
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

auto SyncPrimitiveAllocator::acquireFence(Fence** ppFence, bool isSignaled) -> Result
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> lock{ m_fenceLock };
    auto& pFence = *ppFence;

    // See if there's a free fence available.
    if (!m_availableFences.empty())
    {
        auto vkFence = m_availableFences.front()->getHandle();
        pFence       = m_fencePool.allocate(m_pDevice, vkFence);
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

auto SyncPrimitiveAllocator::releaseFence(Fence* pFence) -> Result
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

auto SyncPrimitiveAllocator::Exists(Fence* pFence) -> bool
{
    std::lock_guard<std::mutex> lock{ m_fenceLock };

    auto result = (m_allFences.find(pFence) != m_allFences.end());

    return result;
}

auto SyncPrimitiveAllocator::acquireSemaphore(uint32_t semaphoreCount, Semaphore** ppSemaphores) -> Result
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> lock{ m_semaphoreLock };

    // See if there are free semaphores available.
    while (!m_availableSemaphores.empty())
    {
        auto& pSemaphore        = *ppSemaphores;
        VkSemaphore vkSemaphore = m_availableSemaphores.front()->getHandle();
        pSemaphore              = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
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
        auto [result, vkSemaphore]           = m_pDevice->getHandle().createSemaphore(createInfo, vk_allocator());
        ppSemaphores[i]                      = m_semaphorePool.allocate(m_pDevice, vkSemaphore);
        if (result != ::vk::Result::eSuccess)
        {
            return { Result::RuntimeError, "Failed to acquire semaphore." };
        }

        m_allSemaphores.emplace(ppSemaphores[i]);
    }

    return Result::Success;
}

auto SyncPrimitiveAllocator::ReleaseSemaphores(uint32_t semaphoreCount, Semaphore** ppSemaphores) -> Result
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

auto SyncPrimitiveAllocator::Exists(Semaphore* semaphore) -> bool
{
    std::lock_guard<std::mutex> lock{ m_semaphoreLock };
    auto result = m_allSemaphores.contains(semaphore);
    return result;
}

auto Fence::wait(uint64_t timeout) -> bool
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
}

auto SyncPrimitiveAllocator::clear() -> void
{
    // Destroy all created fences.
    {
        std::lock_guard<std::mutex> lock{ m_fenceLock };
        for (auto* fence : m_allFences)
        {
            m_pDevice->getHandle().destroyFence(fence->getHandle(), vk_allocator());
            m_fencePool.free(fence);
        }
        m_allFences.clear();
    }

    // Destroy all created semaphores.
    {
        std::lock_guard<std::mutex> lock{ m_semaphoreLock };
        for (auto* semaphore : m_allSemaphores)
        {
            m_pDevice->getHandle().destroySemaphore(semaphore->getHandle(), vk_allocator());
            m_semaphorePool.free(semaphore);
        }
        m_allSemaphores.clear();
    }
}
} // namespace aph::vk
