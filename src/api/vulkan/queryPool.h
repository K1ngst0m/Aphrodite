#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct QueryPoolCreateInfo
{
    QueryType type            = QueryType::Occlusion;
    uint32_t queryCount       = 0;
    PipelineStatisticsFlags statisticsFlags = {};
};

class QueryPool : public ResourceHandle<::vk::QueryPool, QueryPoolCreateInfo>
{
    friend class ThreadSafeObjectPool<QueryPool>;

public:
    auto getQueryType() const -> QueryType;
    auto getQueryCount() const -> uint32_t;
    auto getStatisticsFlags() const -> PipelineStatisticsFlags;

private:
    QueryPool(const CreateInfoType& createInfo, HandleType handle);
};
} // namespace aph::vk 