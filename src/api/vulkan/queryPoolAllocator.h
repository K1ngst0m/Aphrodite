#pragma once

#include "forward.h"
#include "queryPool.h"
#include "common/common.h"

namespace aph::vk
{
struct QueryPoolAllocationConfig
{
    uint32_t timestampPoolCount = 32;        // Number of timestamp query pools
    uint32_t timestampQueryCount = 128;      // Queries per timestamp pool
    
    uint32_t occlusionPoolCount = 8;         // Number of occlusion query pools
    uint32_t occlusionQueryCount = 64;       // Queries per occlusion pool
    
    uint32_t pipelineStatsPoolCount = 4;     // Number of pipeline statistics query pools
    uint32_t pipelineStatsQueryCount = 32;   // Queries per pipeline stats pool
    
    PipelineStatisticsFlags pipelineStatisticsFlags = 
        PipelineStatistic::InputAssemblyVertices | 
        PipelineStatistic::VertexShaderInvocations |
        PipelineStatistic::FragmentShaderInvocations;
};

class QueryPoolAllocator
{
public:
    explicit QueryPoolAllocator(Device* pDevice);
    ~QueryPoolAllocator();
    
    QueryPoolAllocator(const QueryPoolAllocator&) = delete;
    QueryPoolAllocator& operator=(const QueryPoolAllocator&) = delete;
    QueryPoolAllocator(QueryPoolAllocator&&) = delete;
    QueryPoolAllocator& operator=(QueryPoolAllocator&&) = delete;
    
    auto initialize(const QueryPoolAllocationConfig& config = {}) -> Result;
    auto acquire(QueryType type) -> QueryPool*;
    auto release(QueryPool* pQueryPool) -> Result;
    auto resetAll(QueryType type, CommandBuffer* pCommandBuffer) -> void;
    
private:
    auto allocateQueryPools(QueryType type, uint32_t queryCount, uint32_t poolCount, PipelineStatisticsFlags statsFlags = {}) -> Result;
    
private:
    Device* m_pDevice = nullptr;
    
    struct TypedPoolStorage
    {
        SmallVector<QueryPool*> allocated;
        SmallVector<QueryPool*> free;
    };
    
    HashMap<QueryType, TypedPoolStorage> m_pools;
};
} // namespace aph::vk 