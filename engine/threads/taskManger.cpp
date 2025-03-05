#include <utility>

#include "common/common.h"
#include "common/logger.h"
#include "taskManager.h"
#include "threadUtils.h"

#ifdef APH_DEBUG
namespace
{
template <typename... Args>
void logThreadDebug(std::string_view fmt, Args&&... args)
{
    std::ostringstream ss;
    ss << "[THREAD: " << aph::thread::getName().c_str() << "] ";
    ss << fmt;
    ::aph::Logger::GetInstance().debug(ss.str(), std::forward<Args>(args)...);
}

#define THREAD_LOG_DEBUG(...)        \
    do                               \
    {                                \
        logThreadDebug(__VA_ARGS__); \
    } while (0)
} // namespace
#else
#define THREAD_LOG_DEBUG(...) (void(0));
#endif

#ifndef APH_TASK_MANAGER_THREAD_COUNT
#define APH_TASK_MANAGER_THREAD_COUNT 1
#endif
namespace aph::detail
{
TaskManager DefaultTaskManager = { APH_TASK_MANAGER_THREAD_COUNT, "Default Task Manager" };
}

namespace aph
{

TaskManager::TaskManager(uint32_t threadCount, std::string description)
    : m_description(std::move(description))
{
    m_totalTaskCount.store(0);
    m_completedTaskCount.store(0);

    if (threadCount == 0)
    {
        threadCount = std::thread::hardware_concurrency();
    }

    CM_LOG_INFO("Task Manager [%s] init, thread count: %u.", m_description, threadCount);

    m_threadData.threadPool = std::make_unique<ThreadPool<>>(threadCount);

    for (auto idx = 0; idx < threadCount; idx++)
    {
        auto&& res = m_threadData.threadPool->enqueue([this, idx]() { this->processTask(idx); });
        m_threadData.threadResults.push_back(std::move(res));
    }
}

TaskManager::~TaskManager()
{
    wait();

    {
        std::lock_guard<std::mutex> holder{ m_threadData.condLock };
        m_dead = true;
        m_threadData.cond.notify_all();
    }

    for (auto& result : m_threadData.threadResults)
    {
        result.wait();
    }

    m_taskDepsPool.clear();
    m_taskGroupPool.clear();
    m_taskPool.clear();
}

TaskGroup* TaskManager::createTaskGroup(std::string desc)
{
    if (desc.empty())
    {
        desc = m_description + ": Untitled Group";
    }

    CM_LOG_DEBUG("create task group [%s]", desc);
    auto group = m_taskGroupPool.allocate(this, std::move(desc));
    group->m_pDeps = m_taskDepsPool.allocate(this);
    group->m_pDeps->m_pendingTaskCount.store(0, std::memory_order_relaxed);
    return group;
}

Task* TaskManager::addTask(TaskGroup* pGroup, TaskFunc&& func, std::string desc)
{
    if (desc.empty())
    {
        desc = m_description + ": Untitled Task";
    }

    CM_LOG_DEBUG("add task [%s]", desc);
    Task* task = m_taskPool.allocate(pGroup->m_pDeps, std::move(func), std::move(desc));
    pGroup->m_pDeps->m_pendingTasks.push_back(task);
    pGroup->m_pDeps->m_pendingTaskCount.fetch_add(1, std::memory_order_relaxed);
    return task;
}

void TaskManager::removeTaskGroup(TaskGroup* pGroup)
{
    CM_LOG_DEBUG("free task group [%s]", pGroup->m_desc);
    m_taskGroupPool.free(pGroup);
}

void TaskManager::setDependency(TaskGroup* pDependee, TaskGroup* pDependency)
{
    CM_LOG_DEBUG("set dependency [%s -> %s]", pDependency->m_desc, pDependee->m_desc);
    pDependency->m_pDeps->m_pendingDeps.push_back(pDependee->m_pDeps);
    pDependee->m_pDeps->m_dependencyCount.fetch_add(1, std::memory_order_relaxed);
}

void TaskManager::submit(TaskGroup* pGroup)
{
    CM_LOG_DEBUG("submit taskgroup [%s]", pGroup->m_desc);
    pGroup->flush();
    removeTaskGroup(pGroup);
}

TaskGroup::TaskGroup(TaskManager* manager, std::string desc)
    : m_pManager(manager)
    , m_desc(std::move(desc))
{
}

TaskGroup::~TaskGroup()
{
    if (!m_flushed)
    {
        flush();
    }
}

void TaskGroup::submit()
{
    m_pManager->submit(this);
}

void TaskGroup::flush()
{
    CM_LOG_DEBUG("task group flush [%s]", m_desc);
    if (m_flushed)
    {
        CM_LOG_WARN("The task group has been already flushed.");
        return;
    }

    m_flushed = true;
    m_pDeps->dependencySatisfied();
}

void TaskGroup::wait()
{
    CM_LOG_DEBUG("task group wait [%s]", m_desc);
    if (!m_flushed)
    {
        flush();
    }

    std::unique_lock<std::mutex> holder{ m_pDeps->m_condLock };
    m_pDeps->m_cond.wait(holder, [this]() { return m_pDeps->m_done; });
}

bool TaskGroup::poll()
{
    CM_LOG_DEBUG("task group poll [%s]", m_desc);
    if (!m_flushed)
    {
        flush();
    }
    return m_pDeps->m_pendingTaskCount.load(std::memory_order_acquire) == 0;
}

Task* TaskGroup::addTask(TaskFunc&& func, std::string desc)
{
    return m_pManager->addTask(this, std::move(func), std::move(desc));
}

TaskDeps::TaskDeps(TaskManager* manager)
    : m_pManager(manager)
{
    m_pendingTaskCount.store(0, std::memory_order_relaxed);
    // One implicit dependency is the flush() happening.
    m_dependencyCount.store(1, std::memory_order_relaxed);
}
void TaskDeps::taskCompleted()
{
    THREAD_LOG_DEBUG("task deps completed.");
    auto old_tasks = m_pendingTaskCount.fetch_sub(1, std::memory_order_acq_rel);
    APH_ASSERT(old_tasks > 0);
    if (old_tasks == 1)
    {
        notifyDependees();
    }
}
void TaskDeps::notifyDependees()
{
    THREAD_LOG_DEBUG("notify dependees.");
    for (auto& dep : m_pendingDeps)
    {
        dep->dependencySatisfied();
    }

    m_pendingDeps.clear();

    {
        std::lock_guard<std::mutex> holder{ m_condLock };
        m_done = true;
        m_cond.notify_all();
    }
}
void TaskDeps::dependencySatisfied()
{
    auto old_deps = m_dependencyCount.fetch_sub(1, std::memory_order_acq_rel);
    APH_ASSERT(old_deps > 0);

    if (old_deps == 1)
    {
        if (m_pendingTasks.empty())
        {
            notifyDependees();
        }
        else
        {
            m_pManager->scheduleTasks(m_pendingTasks);
            m_pendingTasks.clear();
        }
    }
}
void TaskManager::scheduleTasks(const SmallVector<Task*>& taskList)
{
    unsigned taskCount = taskList.size();

    m_totalTaskCount.fetch_add(taskCount, std::memory_order_relaxed);

    if (taskCount)
    {
        std::lock_guard<std::mutex> holder{ m_threadData.condLock };

        for (auto& t : taskList)
        {
            THREAD_LOG_DEBUG("push task [%s] to ready queue.", t->m_desc);
            m_threadData.readyTaskQueue.push(t);
        }

        for (unsigned i = 0; i < taskCount; i++)
        {
            m_threadData.cond.notify_one();
        }
    }
}

void TaskManager::processTask(uint32_t id)
{
    aph::thread::setName(m_description.substr(0, 12) + ":" + std::to_string(id));

    while (true)
    {
        auto& ctx = m_threadData;
        Task* task = nullptr;

        // grab the task from ready task queue
        {
            std::unique_lock<std::mutex> lock{ ctx.condLock };
            ctx.cond.wait(lock, [&]() { return m_dead || !ctx.readyTaskQueue.empty(); });

            if (m_dead && ctx.readyTaskQueue.empty())
            {
                THREAD_LOG_DEBUG("Task manager is shutdown and all tasks has completed.");
                break;
            }

            task = ctx.readyTaskQueue.front();
            ctx.readyTaskQueue.pop();
        }

        THREAD_LOG_DEBUG("running task [%s]", task->m_desc);

        task->invoke();

        task->m_pDeps->taskCompleted();
        m_taskPool.free(task);

        // check if all tasks complete
        {
            auto completed = m_completedTaskCount.fetch_add(1, std::memory_order_relaxed) + 1;
            if (completed == m_totalTaskCount.load(std::memory_order_relaxed))
            {
                std::lock_guard<std::mutex> holder{ m_waitCondLock };
                m_waitCond.notify_all();
            }
        }
    }
}
void TaskManager::wait()
{
    std::unique_lock<std::mutex> holder{ m_waitCondLock };
    m_waitCond.wait(holder,
                    [&]()
                    {
                        return m_totalTaskCount.load(std::memory_order_relaxed) ==
                               m_completedTaskCount.load(std::memory_order_relaxed);
                    });
}
} // namespace aph
