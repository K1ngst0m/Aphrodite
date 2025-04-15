#include "queryPoolAllocator.h"
#include "commandBuffer.h"
#include "common/profiler.h"
#include "vkUtils.h"
#include "device.h"

namespace aph::vk
{

QueryPoolAllocator::QueryPoolAllocator(Device* pDevice)
    : m_pDevice(pDevice)
{
    APH_ASSERT(pDevice, "Device cannot be null");
}

QueryPoolAllocator::~QueryPoolAllocator()
{
    for (auto& [type, storage] : m_pools)
    {
        for (auto* pPool : storage.allocated)
        {
            if (pPool)
            {
                m_pDevice->destroy(pPool);
            }
        }

        for (auto* pPool : storage.free)
        {
            if (pPool)
            {
                m_pDevice->destroy(pPool);
            }
        }
    }

    m_pools.clear();
}

auto QueryPoolAllocator::initialize(const QueryPoolAllocationConfig& config) -> Result
{
    APH_PROFILER_SCOPE();

    CM_LOG_INFO("Initializing QueryPoolAllocator with config:");
    CM_LOG_INFO("  Timestamp pools: %u pools × %u queries", config.timestampPoolCount, config.timestampQueryCount);
    CM_LOG_INFO("  Occlusion pools: %u pools × %u queries", config.occlusionPoolCount, config.occlusionQueryCount);
    CM_LOG_INFO("  Pipeline stats pools: %u pools × %u queries", config.pipelineStatsPoolCount,
                config.pipelineStatsQueryCount);

    if (config.timestampPoolCount > 0)
    {
        auto result = allocateQueryPools(QueryType::Timestamp, config.timestampQueryCount, config.timestampPoolCount);
        if (!result.success())
        {
            CM_LOG_ERR("Failed to allocate timestamp query pools: %s", result.toString());
            return result;
        }
    }

    if (config.occlusionPoolCount > 0)
    {
        auto result = allocateQueryPools(QueryType::Occlusion, config.occlusionQueryCount, config.occlusionPoolCount);
        if (!result.success())
        {
            CM_LOG_ERR("Failed to allocate occlusion query pools: %s", result.toString());
            return result;
        }
    }

    if (config.pipelineStatsPoolCount > 0)
    {
        auto result = allocateQueryPools(QueryType::PipelineStatistics, config.pipelineStatsQueryCount,
                                         config.pipelineStatsPoolCount, config.pipelineStatisticsFlags);
        if (!result.success())
        {
            CM_LOG_ERR("Failed to allocate pipeline statistics query pools: %s", result.toString());
            return result;
        }
    }

    return Result::Success;
}

auto QueryPoolAllocator::allocateQueryPools(QueryType type, uint32_t queryCount, uint32_t poolCount,
                                            PipelineStatisticsFlags statsFlags) -> Result
{
    APH_PROFILER_SCOPE();

    if (!m_pools.contains(type))
    {
        m_pools[type] = TypedPoolStorage{};
    }

    auto& storage = m_pools[type];

    for (uint32_t i = 0; i < poolCount; ++i)
    {
        QueryPoolCreateInfo createInfo{ .type = type, .queryCount = queryCount, .statisticsFlags = statsFlags };

        auto result = m_pDevice->create(createInfo, std::format("QueryPool_{}_{}", utils::toString(type), i));

        if (!result.success())
        {
            return result;
        }

        storage.free.push_back(result.value());
    }

    CM_LOG_INFO("Allocated %u query pools of type %s with %u queries each", poolCount, utils::toString(type),
                queryCount);

    return Result::Success;
}

auto QueryPoolAllocator::acquire(QueryType type) -> QueryPool*
{
    APH_PROFILER_SCOPE();

    if (!m_pools.contains(type) || m_pools[type].free.empty())
    {
        CM_LOG_WARN("No available query pools of type %s", utils::toString(type));
        return nullptr;
    }

    auto& storage = m_pools[type];

    QueryPool* pPool = storage.free.back();
    storage.free.pop_back();
    storage.allocated.push_back(pPool);

    return pPool;
}

auto QueryPoolAllocator::release(QueryPool* pQueryPool) -> Result
{
    APH_PROFILER_SCOPE();

    if (!pQueryPool)
    {
        return Result::Success;
    }

    QueryType type = pQueryPool->getQueryType();

    if (!m_pools.contains(type))
    {
        return { Result::RuntimeError, "Unknown query pool type" };
    }

    auto& storage = m_pools[type];

    auto it = std::ranges::find(storage.allocated, pQueryPool);
    if (it == storage.allocated.end())
    {
        return { Result::RuntimeError, "Query pool not found in allocated list" };
    }

    storage.allocated.erase(it);
    storage.free.push_back(pQueryPool);

    return Result::Success;
}

auto QueryPoolAllocator::resetAll(QueryType type, CommandBuffer* pCommandBuffer) -> void
{
    APH_PROFILER_SCOPE();

    if (!m_pools.contains(type))
    {
        CM_LOG_WARN("No query pools of type %s to reset", utils::toString(type));
        return;
    }

    auto& storage = m_pools[type];

    for (auto* pPool : storage.allocated)
    {
        if (pPool)
        {
            pCommandBuffer->resetQueryPool(pPool, 0, pPool->getQueryCount());
        }
    }

    for (auto* pPool : storage.free)
    {
        if (pPool)
        {
            pCommandBuffer->resetQueryPool(pPool, 0, pPool->getQueryCount());
        }
    }
}
} // namespace aph::vk
