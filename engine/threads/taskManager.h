#pragma once

#include <utility>

#include "allocator/objectPool.h"
#include "common/common.h"
#include "common/singleton.h"
#include "common/smallVector.h"
#include "threadPool.h"

namespace aph
{
using TaskFunc = std::variant<std::function<Result()>, std::function<void()>>;

struct Task;
class TaskManager;

class TaskDeps
{
    friend class ObjectPool<TaskDeps>;
    friend class TaskManager;
    friend class TaskGroup;

public:
    void taskCompleted();
    void dependencySatisfied();
    void notifyDependees();

private:
    explicit TaskDeps(TaskManager* manager);

    SmallVector<TaskDeps*> m_pendingDeps;
    std::atomic_uint m_pendingTaskCount;

    SmallVector<Task*> m_pendingTasks;
    std::atomic_uint m_dependencyCount;

    std::condition_variable m_cond;
    std::mutex m_condLock;
    bool m_done = false;

    TaskManager* m_pManager = {};
};

struct Task
{
    friend class ObjectPool<Task>;

    std::future<Result> getResult()
    {
        return m_promise.get_future();
    }

    void invoke()
    {
        Result result = Result::Success;
        std::visit(
            [&result](auto&& callable)
            {
                using ReturnType = decltype(callable());
                if constexpr (std::is_same_v<ReturnType, Result>)
                {
                    result = callable();
                }
                else if constexpr (std::is_same_v<ReturnType, void>)
                {
                    callable();
                }
            },
            m_callable);
        m_promise.set_value(result);
    }

    TaskDeps* m_pDeps = {};
    std::string m_desc = {};

private:
    Task(TaskDeps* pDeps, TaskFunc&& func, std::string desc)
        : m_pDeps(pDeps)
        , m_desc(std::move(desc))
        , m_callable(std::forward<TaskFunc>(func))
    {
    }

    std::promise<Result> m_promise;
    TaskFunc m_callable = {};
};

class TaskGroup
{
    friend class TaskManager;
    friend class ObjectPool<TaskGroup>;

public:
    ~TaskGroup();
    void submit();
    void flush();
    void wait();
    bool poll();
    Task* addTask(TaskFunc&& func, std::string desc = "");

private:
    explicit TaskGroup(TaskManager* manager, std::string desc);
    TaskManager* m_pManager = {};
    TaskDeps* m_pDeps = {};
    std::string m_desc = {};
    bool m_flushed = { false };
};

class TaskManager final
{
public:
    TaskManager(uint32_t threadCount = 0, std::string description = {});
    ~TaskManager();

    TaskGroup* createTaskGroup(std::string desc = "");
    void removeTaskGroup(TaskGroup* pGroup);
    void setDependency(TaskGroup* pDependee, TaskGroup* pDependency);

    void scheduleTasks(const SmallVector<Task*>& taskList);

    Task* addTask(TaskGroup* pGroup, TaskFunc&& func, std::string desc = "");

    void submit(TaskGroup* pGroup);

    void wait();

private:
    void processTask(uint32_t id);

    bool m_dead = false;

    std::condition_variable m_waitCond;
    std::mutex m_waitCondLock;
    std::atomic_uint m_totalTaskCount;
    std::atomic_uint m_completedTaskCount;

    struct
    {
        SmallVector<std::future<void>> threadResults;
        std::unique_ptr<ThreadPool<>> threadPool;
        std::queue<Task*> readyTaskQueue;
        std::mutex condLock;
        std::condition_variable cond;
    } m_threadData;

private:
    ThreadSafeObjectPool<Task> m_taskPool;
    ThreadSafeObjectPool<TaskGroup> m_taskGroupPool;
    ThreadSafeObjectPool<TaskDeps> m_taskDepsPool;

private:
    std::string m_description;
};

} // namespace aph

namespace aph::detail
{
extern TaskManager DefaultTaskManager;
}

#define APH_DEFAULT_TASK_MANAGER aph::detail::DefaultTaskManager
