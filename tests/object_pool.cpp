#include <catch2/catch_test_macros.hpp>
#
#include <thread>
#include <set>
#include "common/objectPool.h"

// - Test allocation and deallocation for single objects.
// - Test allocation and deallocation for multiple objects.
// - Test clear functionality.
// - For the ThreadSafeObjectPool, perform multi-threaded tests.

using namespace aph;

// This is a sample object for the object pool
struct TestObject
{
    int value;

    TestObject() : value(0) {}
    explicit TestObject(int v) : value(v) {}
};

TEST_CASE("ObjectPool functionality", "[ObjectPool]")
{
    ObjectPool<TestObject> pool;

    SECTION("Single object allocation and deallocation")
    {
        TestObject* obj = pool.allocate();
        REQUIRE(obj != nullptr);
        obj->value = 10;
        REQUIRE(obj->value == 10);
        pool.free(obj);
    }

    SECTION("Multiple object allocation and deallocation")
    {
        std::vector<TestObject*> objects;
        for(int i = 0; i < 100; ++i)
        {
            TestObject* obj = pool.allocate(i);
            REQUIRE(obj != nullptr);
            REQUIRE(obj->value == i);
            objects.push_back(obj);
        }

        for(auto obj : objects)
        {
            pool.free(obj);
        }
    }

    SECTION("Clearing the pool")
    {
        TestObject* obj = pool.allocate();
        REQUIRE(obj != nullptr);
        pool.free(obj);

        pool.clear();
        // After clearing, we should still be able to allocate an object.
        obj = pool.allocate();
        REQUIRE(obj != nullptr);
        pool.free(obj);
    }
}

TEST_CASE("ThreadSafeObjectPool functionality", "[ThreadSafeObjectPool]")
{
    ThreadSafeObjectPool<TestObject> pool;

    SECTION("Single object allocation and deallocation")
    {
        TestObject* obj = pool.allocate();
        REQUIRE(obj != nullptr);
        obj->value = 10;
        REQUIRE(obj->value == 10);
        pool.free(obj);
    }

    SECTION("Multiple object allocation and deallocation")
    {
        std::vector<TestObject*> objects;
        for(int i = 0; i < 100; ++i)
        {
            TestObject* obj = pool.allocate(i);
            REQUIRE(obj != nullptr);
            REQUIRE(obj->value == i);
            objects.push_back(obj);
        }

        for(auto obj : objects)
        {
            pool.free(obj);
        }
    }

    SECTION("Clearing the pool")
    {
        TestObject* obj = pool.allocate();
        REQUIRE(obj != nullptr);
        pool.free(obj);

        pool.clear();
        // After clearing, we should still be able to allocate an object.
        obj = pool.allocate();
        REQUIRE(obj != nullptr);
        pool.free(obj);
    }

    SECTION("Multithreaded allocation and deallocation")
    {
        std::atomic<int>         counter(0);
        std::vector<std::thread> threads;

        threads.reserve(10);
        for(int i = 0; i < 10; ++i)
        {
            threads.emplace_back([&pool, &counter]() {
                for(int j = 0; j < 100; ++j)
                {
                    TestObject* obj = pool.allocate(j);
                    REQUIRE(obj != nullptr);
                    REQUIRE(obj->value == j);
                    pool.free(obj);
                    counter++;
                }
            });
        }

        for(auto& t : threads)
        {
            t.join();
        }

        REQUIRE(counter == 1000);
    }
}
