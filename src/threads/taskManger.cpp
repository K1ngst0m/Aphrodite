#include "taskManager.h"
#include "threadUtils.h"

#include "common/common.h"
#include "common/logger.h"
#include "common/profiler.h"

#include <utility>

namespace aph
{

void TaskGroup::addTask(coro::task<Result> task)
{
    m_tasks.push_back(std::move(task));
}

std::future<Result> TaskGroup::submitAsync()
{
    return m_pTaskManager->submit(this);
}

Result TaskGroup::submit()
{
    return submitAsync().get();
}

void TaskGroup::waitFor(TaskGroup* pGroup)
{
    m_pTaskManager->setDependencies(this, pGroup);
}

TaskManager::TaskManager(uint32_t threadCount)
    : m_threadPool(coro::thread_pool::options{.thread_count = threadCount,
                                              .on_thread_start_functor = [](std::size_t worker_idx) -> void
                                              { CM_LOG_DEBUG("thread pool worker %u is starting up.", worker_idx); },
                                              .on_thread_stop_functor = [](std::size_t worker_idx) -> void
                                              { CM_LOG_DEBUG("thread pool worker %u is shutting down.", worker_idx); }})
{
}

void TaskManager::addTask(TaskGroup* pGroup, coro::task<Result> task)
{
    pGroup->addTask(std::move(task));
}

std::future<Result> TaskManager::submit(TaskGroup* pGroup)
{
    APH_PROFILER_SCOPE();
    auto taskDoneLatch = std::make_shared<coro::latch>(pGroup->m_tasks.size());

    SmallVector<TaskType> pendingTasks;

    for (auto&& task : pGroup->m_tasks)
    {
        auto taskWrapper = [](coro::thread_pool& tp, coro::task<Result> task, const coro::latch& waitLatch,
                              std::shared_ptr<coro::latch> signalLatch) -> TaskType
        {
            co_await tp.schedule();
            co_await waitLatch;
            auto result = co_await tp.schedule(std::move(task));
            signalLatch->count_down();
            co_return result;
        };

        pendingTasks.emplace_back(taskWrapper(m_threadPool, std::move(task), pGroup->m_waitLatch, taskDoneLatch));
    }

    for (auto* pendingGroup : pGroup->m_pendingGroups)
    {
        auto signalTask = [](coro::thread_pool& tp, TaskGroup* pPendingGroup,
                             std::shared_ptr<coro::latch> waitLatch) -> TaskType
        {
            co_await tp.schedule();
            co_await *waitLatch;
            pPendingGroup->m_waitLatch.count_down();
            co_return Result::Success;
        };

        pendingTasks.emplace_back(signalTask(m_threadPool, pendingGroup, taskDoneLatch));
    }

    auto promise = std::make_shared<std::promise<Result>>();
    auto submitTask = [](SmallVector<TaskType> tasks, std::shared_ptr<std::promise<Result>> promise) -> coro::task<void>
    {
        ResultGroup resultGroup{};
        auto results = coro::sync_wait(coro::when_all(std::move(tasks)));
        for (const auto& result : results)
        {
            resultGroup += std::move(result.return_value());
        }
        promise->set_value(resultGroup);
        co_return;
    };

    m_threadPool.spawn(submitTask(std::move(pendingTasks), promise));

    pGroup->m_tasks.clear();
    return promise->get_future();
}

void TaskManager::setDependencies(TaskGroup* pProducer, TaskGroup* pConsumer)
{
    pProducer->m_pendingGroups.insert(pConsumer);
    // TODO check if UB
    pConsumer->m_waitLatch.count_down(-1);
}

TaskManager::~TaskManager()
{
    for (const auto& [group, _] : m_pendingTasks)
    {
        m_taskGroupPools.free(group);
    }
    m_taskGroupPools.clear();
}
} // namespace aph
