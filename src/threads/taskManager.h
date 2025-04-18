#pragma once

#include "allocator/objectPool.h"
#include "common/common.h"
#include "common/hash.h"
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
    friend class ThreadSafeObjectPool<TaskGroup>;

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

    void cleanup();

    template <typename TStr>
    TaskGroup* createTaskGroup(TStr&& name = {})
    {
        auto* pGroup           = m_taskGroupPools.allocate(this, APH_FWD(name));
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
