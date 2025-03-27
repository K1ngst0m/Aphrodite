#pragma once

#include <atomic>
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
#include "common/hash.h"
#include "common/singleton.h"
#include "common/smallVector.h"

namespace aph
{
using TaskType = coro::task<Result>;
class TaskManager;

class TaskGroup
{
public:
    void addTask(TaskType task);
    std::future<Result> submitAsync();
    Result submit();
    void waitFor(TaskGroup* pGroup);

private:
    friend class TaskManager;
    friend class ObjectPool<TaskGroup>;
    TaskGroup(TaskManager* pTaskManager, auto&& name)
        : m_pTaskManager(pTaskManager)
        , m_name(APH_FWD(name))
    {
    }

    SmallVector<TaskType> m_tasks;
    TaskManager* m_pTaskManager = {};
    std::string m_name;
    HashSet<TaskGroup*> m_pendingGroups;
    coro::latch m_waitLatch{ 0 };
};

class TaskManager
{
public:
    TaskManager(uint32_t threadCount = std::thread::hardware_concurrency());
    ~TaskManager();

    template <typename TStr>
    TaskGroup* createTaskGroup(TStr&& name = {})
    {
        auto* pGroup = m_taskGroupPools.allocate(this, APH_FWD(name));
        m_pendingTasks[pGroup] = {};
        return pGroup;
    }

    void addTask(TaskGroup* pGroup, TaskType task);
    std::future<Result> submit(TaskGroup* pGroup);
    void setDependencies(TaskGroup* pProducer, TaskGroup* pConsumer);

private:
    HashMap<TaskGroup*, SmallVector<TaskType>> m_pendingTasks;
    coro::thread_pool m_threadPool{};
    ThreadSafeObjectPool<TaskGroup> m_taskGroupPools;
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
