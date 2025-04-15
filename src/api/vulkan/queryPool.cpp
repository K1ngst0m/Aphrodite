#include "queryPool.h"

namespace aph::vk
{

QueryPool::QueryPool(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
{
}

auto QueryPool::getQueryType() const -> QueryType
{
    return m_createInfo.type;
}

auto QueryPool::getQueryCount() const -> uint32_t
{
    return m_createInfo.queryCount;
}

auto QueryPool::getStatisticsFlags() const -> PipelineStatisticsFlags
{
    return m_createInfo.statisticsFlags;
}

} // namespace aph::vk