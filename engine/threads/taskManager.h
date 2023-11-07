#ifndef APH_JOBSYSTEM_H_
#define APH_JOBSYSTEM_H_

#include <utility>

#include "common/singleton.h"
#include "allocator/objectPool.h"
#include "threadPool.h"

namespace aph
{
using TaskFunc = std::function<void()>;

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

    std::vector<TaskDeps*> m_pendingDeps;
    std::atomic_uint       m_pendingTaskCount;

    std::vector<Task*> m_pendingTasks;
    std::atomic_uint   m_dependencyCount;

    std::condition_variable m_cond;
    std::mutex              m_condLock;
    bool                    m_done = false;

    TaskManager* m_pManager = {};
};

struct Task
{
    friend class ObjectPool<Task>;

    TaskFunc    m_callable = {};
    TaskDeps*   m_pDeps    = {};
    std::string m_desc     = {};

private:
    Task(TaskDeps* pDeps, TaskFunc&& func, std::string desc) :
        m_callable(std::forward<TaskFunc>(func)),
        m_pDeps(pDeps),
        m_desc(std::move(desc))
    {
    }
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
    void addTask(TaskFunc&& func, const std::string& desc = "untitled");

private:
    explicit TaskGroup(TaskManager* manager, std::string desc);
    TaskManager* m_pManager = {};
    TaskDeps*    m_pDeps    = {};
    std::string  m_desc     = {};
    bool         m_flushed  = {false};
};

class TaskManager final
{
public:
    TaskManager(uint32_t threadCount = 0, std::string description = {});
    ~TaskManager();

    TaskGroup* createTaskGroup(const std::string& desc = "untitled");
    void       removeTaskGroup(TaskGroup* pGroup);
    void       setDependency(TaskGroup* pDependee, TaskGroup* pDependency);

    void scheduleTasks(const std::vector<Task*>& taskList);

    void addTask(TaskGroup* pGroup, TaskFunc&& func, const std::string& desc = "untitled");

    void submit(TaskGroup* pGroup);

    void wait();

private:
    void processTask(uint32_t id);

    bool m_dead = false;

    std::condition_variable m_waitCond;
    std::mutex              m_waitCondLock;
    std::atomic_uint        m_totalTaskCount;
    std::atomic_uint        m_completedTaskCount;

    struct
    {
        std::vector<std::future<void>> threadResults;
        std::unique_ptr<ThreadPool<>>  threadPool;
        std::queue<Task*>              readyTaskQueue;
        std::mutex                     condLock;
        std::condition_variable        cond;
    } m_threadData;

private:
    ThreadSafeObjectPool<Task>      m_taskPool;
    ThreadSafeObjectPool<TaskGroup> m_taskGroupPool;
    ThreadSafeObjectPool<TaskDeps>  m_taskDepsPool;

private:
    std::string m_description;
};

}  // namespace aph
#endif
