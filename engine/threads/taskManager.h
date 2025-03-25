#pragma once

#include <atomic>
#include <condition_variable>
#include <coro/coro.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "allocator/objectPool.h"
#include "common/common.h"
#include "common/singleton.h"
#include "common/smallVector.h"
#include "threadPool.h"

#include <coro/coro.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace aph
{
using TaskType = coro::task<Result>;

/**
 * SyncToken - A synchronization primitive for waiting on asynchronous operations
 */
class SyncToken
{
public:
    SyncToken(Result result)
        : m_result(std::move(result))
    {
    }

private:
    Result m_result;
};

class TaskManager;

class TaskGroup
{
public:
    ResultGroup wait()
    {
        if (m_tasks.empty())
        {
            return Result::Success;
        }

        ResultGroup resultGroup{};
        auto results = coro::sync_wait(coro::when_all(std::move(m_tasks)));
        for (const auto& result : results)
        {
            resultGroup += std::move(result.return_value());
        }
        return resultGroup;
    }

private:
    friend class TaskManager;
    std::vector<TaskType> m_tasks;
};

class TaskManager
{
public:
    TaskManager(uint32_t threadCount = std::thread::hardware_concurrency())
        : m_threadPool(
              coro::thread_pool::options{ .thread_count = threadCount,
                                          .on_thread_start_functor = [](std::size_t worker_idx) -> void
                                          { CM_LOG_INFO("thread pool worker %u is starting up.", worker_idx); },
                                          .on_thread_stop_functor = [](std::size_t worker_idx) -> void
                                          { CM_LOG_INFO("thread pool worker %u is shutting down.", worker_idx); } })
    {
    }

    TaskGroup* createTaskGroup()
    {
        m_taskGroups.emplace_back();
        return &m_taskGroups.back();
    }

    void addTask(TaskGroup* pGroup, coro::task<Result> task)
    {
        auto taskWrapper = [](coro::thread_pool& tp, coro::task<Result> task) -> coro::task<Result>
        { co_return co_await tp.schedule(std::move(task)); };
        pGroup->m_tasks.emplace_back(taskWrapper(m_threadPool, std::move(task)));
    }

private:
    std::vector<TaskGroup> m_taskGroups;
    coro::thread_pool m_threadPool{};
};

} // namespace aph

namespace aph::internal
{
inline TaskManager& getDefaultTaskManager()
{
    static TaskManager defaultTaskManager;
    return defaultTaskManager;
}
} // namespace aph::internal

#define APH_DEFAULT_TASK_MANAGER ::aph::internal::getDefaultTaskManager()
