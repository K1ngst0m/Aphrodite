#include <catch2/catch_all.hpp>
#include "threads/threadPool.h"

using namespace aph;

TEST_CASE("Basic Functionality")
{
    ThreadPool<> pool(2);

    std::atomic<bool> executed{false};
    pool.enqueueDetach([&]() { executed = true; });

    // Give the task some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(executed.load() == true);
}

TEST_CASE("Return Value")
{
    ThreadPool<> pool(2);

    auto future = pool.enqueue([]() { return 42; });

    REQUIRE(future.get() == 42);
}

TEST_CASE("Exception Handling")
{
    ThreadPool<> pool(2);

    auto future = pool.enqueue([]() -> void { throw std::runtime_error("Oops!"); });

    REQUIRE_THROWS_AS(future.get(), std::runtime_error);
}

TEST_CASE("Multiple Tasks")
{
    ThreadPool<> pool(2);

    std::atomic<int> counter{0};
    for(int i = 0; i < 100; ++i)
    {
        pool.enqueueDetach([&]() { counter++; });
    }

    // Give tasks some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(counter.load() == 100);
}

TEST_CASE("Task Stealing")
{
    ThreadPool<> pool(2);

    std::atomic<int> counter{0};
    auto             outerTaskFuture = pool.enqueue([&]() {
        for(int i = 0; i < 50; ++i)
        {
            pool.enqueueDetach([&]() { counter++; });
        }
    });

    // Wait for the outer task to finish
    outerTaskFuture.wait();

    // Give inner tasks some time to execute (if they haven't already)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(counter.load() == 50);
}

TEST_CASE("Thread Count")
{
    ThreadPool<> pool(4);
    REQUIRE(pool.size() == 4);
}

TEST_CASE("Single Threaded Execution")
{
    ThreadPool<> pool(1);

    std::atomic<int> counter{0};
    for(int i = 0; i < 100; ++i)
    {
        pool.enqueueDetach([&]() { counter++; });
    }

    // Give tasks some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(counter.load() == 100);
}

TEST_CASE("Multi Threaded Execution")
{
    ThreadPool<> pool(4);

    std::atomic<int> counter{0};
    for(int i = 0; i < 1000; ++i)
    {
        pool.enqueueDetach([&]() { counter++; });
    }

    // Give tasks some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(counter.load() == 1000);
}

TEST_CASE("Dynamic Task Addition")
{
    ThreadPool<> pool(4);

    std::atomic<int> counter{0};
    pool.enqueueDetach([&]() {
        for(int i = 0; i < 100; ++i)
        {
            pool.enqueueDetach([&]() { counter++; });
        }
    });

    // Give tasks some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(counter.load() == 100);
}

TEST_CASE("Task Execution Order")
{
    ThreadPool<> pool(1);  // Single thread to maintain order

    std::vector<int> order;
    for(int i = 0; i < 10; ++i)
    {
        pool.enqueueDetach([&order, i]() { order.push_back(i); });
    }

    // Give tasks some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool is_ordered = true;
    for(int i = 0; i < 10; ++i)
    {
        if(order[i] != i)
            is_ordered = false;
    }

    REQUIRE(is_ordered);
}

TEST_CASE("Stress Test")
{
    ThreadPool<> pool(8);

    std::atomic<int> counter{0};
    for(int i = 0; i < 10000; ++i)
    {
        pool.enqueueDetach([&]() { counter++; });
    }

    // Give tasks some time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    REQUIRE(counter.load() == 10000);
}
