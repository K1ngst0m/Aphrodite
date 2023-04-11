#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include "common/common.h"

namespace aph
{
// A thread-safe queue class.
template <typename T>
class ThreadSafeQueue
{
public:
    bool Empty()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while(!m_queue.empty())
            m_queue.pop();
        m_condition.notify_all();
    }

    void Invalidate()
    {
        m_valid = false;
        m_condition.notify_all();
    }

    void Push(const T& item)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        assert(m_valid);
        m_queue.push(item);
        lock.unlock();
        m_condition.notify_one();
    }

    void Push(T&& item)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        assert(m_valid);
        m_queue.push(std::move(item));
        lock.unlock();
        m_condition.notify_one();
    }

    bool Pop(T& item)
    {
        // Wait for an item to be in the queue or the queue to be invalidated.
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this](void) { return !m_queue.empty() || !m_valid; });

        // Ensure queue is still valid since above predicate could fall through.
        if(!m_valid)
            return false;

        // Get the item out of the queue.
        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

private:
    std::atomic_bool        m_valid{ true };
    std::queue<T>           m_queue;
    std::mutex              m_mutex;
    std::condition_variable m_condition;
};

// A ThreadPool class for parallel executions of tasks on across one or more threads.
class ThreadPool
{
public:
    // Task definition for code simplicity purposes.
    using Task = std::function<void()>;

    // Constructs the thread pool and sets the max thread count.
    // No threads are allocated until tasks are added to the queue.
    ThreadPool(uint32_t threadCount);

    // Blocks until all threads have completed.
    ~ThreadPool();

    // Adds a new task to the thread pool and returns a future handle.
    std::shared_future<void> AddTask(Task&& task);

    // Clear any pending tasks.
    void ClearPendingTasks();

    // Waits on all threads to complete (blocking call).
    void Wait();

    // Cancels all pending tasks and waits for threads to complete current tasks.
    void Abort();

private:
    std::stack<std::thread>                     m_threads;
    ThreadSafeQueue<std::packaged_task<void()>> m_tasks;
    std::atomic<uint32_t>                       m_activeThreads{ 0U };
    std::mutex                                  m_threadsCompleteMutex;
    std::condition_variable                     m_threadsCompleteCondition;
};
}  // namespace aph

#endif  // THREADPOOL_H_
