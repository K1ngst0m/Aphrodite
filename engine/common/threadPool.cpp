#include "threadPool.h"

namespace vkl {
ThreadPool::ThreadPool(uint32_t threadCount) {
    // Spawn maxThreadCount threads to execute tasks.
    for (auto i = 0U; i < threadCount; ++i) {
        // All threads execute until task queue becomes invalidated.
        m_threads.emplace([this]() {
            while (true) {
                // Pop the next task off of the queue.
                std::packaged_task<void()> packagedTask;
                if (!m_tasks.Pop(packagedTask))
                    break;

                // Increment the number of active threads.
                ++m_activeThreads;

                // Run the task.
                packagedTask();

                // Decrement the number of active threads.
                --m_activeThreads;

                // Notify any thread waiting on task completition via ThreadPool::Wait().
                m_threadsCompleteCondition.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    // Invalidate task queue so threads stop waiting on new tasks but finish remaining tasks in queue.
    m_tasks.Invalidate();

    // Join all active threads.
    while (!m_threads.empty()) {
        auto &thread = m_threads.top();
        thread.join();
        m_threads.pop();
    }
}

std::shared_future<void> ThreadPool::AddTask(Task &&task) {
    std::packaged_task<void()> packagedTask(std::move(task));
    auto                       future = packagedTask.get_future();
    m_tasks.Push(std::move(packagedTask));
    return future.share();
}

void ThreadPool::ClearPendingTasks() {
    m_tasks.Clear();
}

void ThreadPool::Wait() {
    std::unique_lock<std::mutex> lock(m_threadsCompleteMutex);
    m_threadsCompleteCondition.wait(lock, [this]() { return !m_activeThreads && m_tasks.Empty(); });
}

void ThreadPool::Abort() {
    // Clear any pending items.
    m_tasks.Clear();

    // Invalidate task queue so threads stop waiting on new items.
    m_tasks.Invalidate();

    // Wait for running threads to complete.
    Wait();
}
} // namespace vkl
