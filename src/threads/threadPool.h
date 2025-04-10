#pragma once

#include <atomic>
#include <barrier>
#include <concepts>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <semaphore>
#include <thread>
#include <type_traits>
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#include "threadSafeQueue.h"

namespace aph
{
namespace threads
{
#ifdef __cpp_lib_move_only_function
using DefaultFunctionType = std::move_only_function<void()>;
#else
using DefaultFunctionType = std::function<void()>;
#endif
} // namespace threads

template <typename FunctionType = threads::DefaultFunctionType, typename ThreadType = std::jthread>
    requires std::invocable<FunctionType> && std::is_same_v<void, std::invoke_result_t<FunctionType>>
class ThreadPool
{
public:
    explicit ThreadPool(const unsigned int& number_of_threads = std::thread::hardware_concurrency())
        : m_tasks(number_of_threads)
    {
        std::size_t current_id = 0;
        for (std::size_t i = 0; i < number_of_threads; ++i)
        {
            m_priority_queue.push_back(size_t(current_id));
            try
            {
                m_threads.emplace_back(
                    [&, id = current_id](const std::stop_token& stop_tok)
                    {
                        do
                        {
                            // wait until signaled
                            m_tasks[id].signal.acquire();

                            do
                            {
                                // invoke the task
                                while (auto task = m_tasks[id].tasks.pop_front())
                                {
                                    try
                                    {
                                        m_pending_tasks.fetch_sub(1, std::memory_order_release);
                                        std::invoke(std::move(task.value()));
                                    }
                                    catch (...)
                                    {
                                    }
                                }

                                // try to steal a task
                                for (std::size_t j = 1; j < m_tasks.size(); ++j)
                                {
                                    const std::size_t index = (id + j) % m_tasks.size();
                                    if (auto task = m_tasks[index].tasks.steal())
                                    {
                                        // steal a task
                                        m_pending_tasks.fetch_sub(1, std::memory_order_release);
                                        std::invoke(std::move(task.value()));
                                        // stop stealing once we have invoked a stolen task
                                        break;
                                    }
                                }

                            } while (m_pending_tasks.load(std::memory_order_acquire) > 0);

                            m_priority_queue.rotate_to_front(id);

                        } while (!stop_tok.stop_requested());
                    });
                // increment the thread id
                ++current_id;
            }
            catch (...)
            {
                // catch all

                // remove one item from the tasks
                m_tasks.pop_back();

                // remove our thread from the priority queue
                std::ignore = m_priority_queue.pop_back();
            }
        }
    }

    ~ThreadPool()
    {
        // stop all threads
        for (std::size_t i = 0; i < m_threads.size(); ++i)
        {
            m_threads[i].request_stop();
            m_tasks[i].signal.release();
            m_threads[i].join();
        }
    }

    /// thread pool is non-copyable
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Function, typename... Args, typename ReturnType = std::invoke_result_t<Function&&, Args&&...>>
        requires std::invocable<Function, Args...>
    [[nodiscard]] std::future<ReturnType> enqueue(Function f, Args... args)
    {
#ifdef __cpp_lib_move_only_function
        // we can do this in C++23 because we now have support for move only functions
        std::promise<ReturnType> promise;
        auto future = promise.get_future();
        auto task   = [func = std::move(f), ... largs = std::move(args), promise = std::move(promise)]() mutable
        {
            try
            {
                if constexpr (std::is_same_v<ReturnType, void>)
                {
                    func(largs...);
                    promise.set_value();
                }
                else
                {
                    promise.set_value(func(largs...));
                }
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        };
        enqueue_task(std::move(task));
        return future;
#else
        /*
         * use shared promise here so that we don't break the promise later (until C++23)
         *
         * with C++23 we can do the following:
         *
         * std::promise<ReturnType> promise;
         * auto future = promise.get_future();
         * auto task = [func = std::move(f), ...largs = std::move(args),
                          promise = std::move(promise)]() mutable {...};
         */
        auto shared_promise = std::make_shared<std::promise<ReturnType>>();
        auto task           = [func = std::move(f), ... largs = std::move(args), promise = shared_promise]()
        {
            try
            {
                if constexpr (std::is_same_v<ReturnType, void>)
                {
                    func(largs...);
                    promise->set_value();
                }
                else
                {
                    promise->set_value(func(largs...));
                }
            }
            catch (...)
            {
                promise->set_exception(std::current_exception());
            }
        };

        // get the future before enqueuing the task
        auto future = shared_promise->get_future();
        // enqueue the task
        enqueueTask(std::move(task));
        return future;
#endif
    }

    template <typename Function, typename... Args>
        requires std::invocable<Function, Args...> && std::is_same_v<void, std::invoke_result_t<Function&&, Args&&...>>
    void enqueueDetach(Function&& func, Args&&... args)
    {
        enqueueTask(std::move(
            [f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
            {
                // suppress exceptions
                try
                {
                    std::invoke(f, largs...);
                }
                catch (...)
                {
                }
            }));
    }

    [[nodiscard]] auto size() const
    {
        return m_threads.size();
    }

private:
    template <typename Function>
    void enqueueTask(Function&& f)
    {
        auto i_opt = m_priority_queue.copy_front_and_rotate_to_back();
        if (!i_opt.has_value())
        {
            // would only be a problem if there are zero threads
            return;
        }
        auto i = *(i_opt);
        m_pending_tasks.fetch_add(1, std::memory_order_relaxed);
        m_tasks[i].tasks.push_back(std::forward<Function>(f));
        m_tasks[i].signal.release();
    }

    struct TaskItem
    {
        aph::ThreadSafeQueue<FunctionType> tasks{};
        std::binary_semaphore signal{0};
    };

    std::vector<ThreadType> m_threads;
    std::deque<TaskItem> m_tasks;
    aph::ThreadSafeQueue<std::size_t> m_priority_queue;
    std::atomic_int_fast64_t m_pending_tasks{};
};
} // namespace aph
