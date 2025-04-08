#include "allocator/objectPool.h"
#include "allocator/polyObjectPool.h"
#include "common/debug.h"

#include <atomic>
#include <catch2/catch_all.hpp>
#include <mutex>
#include <thread>
#include <vector>

using namespace aph;
using namespace Catch;

// Test classes
class TestObject
{
public:
    TestObject(int value = 0)
        : m_value(value)
        , m_isConstructed(true)
    {
        s_constructCount++;
    }

    ~TestObject()
    {
        m_isConstructed = false;
        s_destructCount++;
    }

    int getValue() const
    {
        return m_value;
    }
    bool isConstructed() const
    {
        return m_isConstructed;
    }

    static void resetStats()
    {
        s_constructCount = 0;
        s_destructCount = 0;
    }

    static int getConstructCount()
    {
        return s_constructCount;
    }
    static int getDestructCount()
    {
        return s_destructCount;
    }

private:
    int m_value;
    bool m_isConstructed;

    static inline std::atomic<int> s_constructCount{0};
    static inline std::atomic<int> s_destructCount{0};
};

// Base class for polymorphic testing
class BaseClass
{
public:
    BaseClass()
    {
        s_constructCount++;
    }
    virtual ~BaseClass()
    {
        s_destructCount++;
    }
    virtual int getType() const = 0;

    // Safe type-checking methods that don't use RTTI
    virtual int getIntValue() const
    {
        return 0;
    }
    virtual float getFloatValue() const
    {
        return 0.0f;
    }

    static void resetStats()
    {
        s_constructCount = 0;
        s_destructCount = 0;
    }

    static int getConstructCount()
    {
        return s_constructCount;
    }
    static int getDestructCount()
    {
        return s_destructCount;
    }

private:
    static inline std::atomic<int> s_constructCount{0};
    static inline std::atomic<int> s_destructCount{0};
};

// Derived classes for polymorphic testing
class DerivedClassA : public BaseClass
{
public:
    DerivedClassA(int val = 1)
        : m_value(val)
    {
    }
    int getType() const override
    {
        return 1;
    }
    int getIntValue() const override
    {
        return m_value;
    }

private:
    int m_value;
};

class DerivedClassB : public BaseClass
{
public:
    DerivedClassB(float val = 2.0f)
        : m_value(val)
    {
    }
    int getType() const override
    {
        return 2;
    }
    float getFloatValue() const override
    {
        return m_value;
    }

private:
    float m_value;
};

// ObjectPool Tests
TEST_CASE("ObjectPool basic functionality", "[objectpool]")
{
    TestObject::resetStats();

    {
        ObjectPool<TestObject> pool;

        SECTION("Allocation and access")
        {
            TestObject* obj1 = pool.allocate(42);
            REQUIRE(obj1 != nullptr);
            REQUIRE(obj1->getValue() == 42);
            REQUIRE(obj1->isConstructed());

            TestObject* obj2 = pool.allocate(100);
            REQUIRE(obj2 != nullptr);
            REQUIRE(obj2->getValue() == 100);
            REQUIRE(obj2 != obj1);

            REQUIRE(pool.getAllocationCount() == 2);
        }

        SECTION("Deallocation")
        {
            TestObject* obj = pool.allocate(42);
            REQUIRE(pool.getAllocationCount() == 1);

            pool.free(obj);
            REQUIRE(pool.getAllocationCount() == 0);

            // Freeing nullptr should be safe
            pool.free(nullptr);
        }

        SECTION("Multiple allocations and deallocations")
        {
            std::vector<TestObject*> objects;

            for (int i = 0; i < 100; i++)
            {
                objects.push_back(pool.allocate(i));
            }

            REQUIRE(pool.getAllocationCount() == 100);

            // Free half the objects
            for (size_t i = 0; i < 50; i++)
            {
                pool.free(objects[i]);
            }

            REQUIRE(pool.getAllocationCount() == 50);

            // Clear should free the rest
            pool.clear();
            REQUIRE(pool.getAllocationCount() == 0);
        }
    }

    // After pool is destroyed, all objects should be freed
    REQUIRE(TestObject::getConstructCount() == TestObject::getDestructCount());
}

// PolymorphicObjectPool Tests
TEST_CASE("PolymorphicObjectPool basic functionality", "[polyobjectpool]")
{
    BaseClass::resetStats();

    {
        PolymorphicObjectPool<BaseClass> pool;

        SECTION("Polymorphic allocation and type verification")
        {
            DerivedClassA* objA = pool.allocate<DerivedClassA>(42);
            REQUIRE(objA != nullptr);
            REQUIRE(objA->getType() == 1);
            REQUIRE(objA->getIntValue() == 42);

            DerivedClassB* objB = pool.allocate<DerivedClassB>(3.14f);
            REQUIRE(objB != nullptr);
            REQUIRE(objB->getType() == 2);
            REQUIRE(objB->getFloatValue() == Approx(3.14f));

            REQUIRE(pool.getAllocationCount() == 2);

            // Free using base pointer
            BaseClass* baseA = objA;
            pool.free(baseA);
            REQUIRE(pool.getAllocationCount() == 1);

            BaseClass* baseB = objB;
            pool.free(baseB);
            REQUIRE(pool.getAllocationCount() == 0);
        }

        SECTION("Multiple allocations of different types")
        {
            std::vector<BaseClass*> objects;

            // Allocate mix of objects
            for (int i = 0; i < 50; i++)
            {
                if (i % 2 == 0)
                {
                    objects.push_back(pool.allocate<DerivedClassA>(i));
                }
                else
                {
                    objects.push_back(pool.allocate<DerivedClassB>(i * 1.5f));
                }
            }

            REQUIRE(pool.getAllocationCount() == 50);

            // Verify correct polymorphic behavior using getType()
            for (size_t i = 0; i < objects.size(); i++)
            {
                if (i % 2 == 0)
                {
                    REQUIRE(objects[i]->getType() == 1);
                    // Verify using virtual methods instead of dynamic_cast
                    REQUIRE(objects[i]->getIntValue() == static_cast<int>(i));
                }
                else
                {
                    REQUIRE(objects[i]->getType() == 2);
                    // Verify using virtual methods instead of dynamic_cast
                    REQUIRE(objects[i]->getFloatValue() == Approx(i * 1.5f));
                }
            }

            // Clear should free all objects
            pool.clear();
            REQUIRE(pool.getAllocationCount() == 0);
        }
    }

    // All objects should be properly destroyed
    REQUIRE(BaseClass::getConstructCount() == BaseClass::getDestructCount());
}

// ThreadSafeObjectPool Tests
TEST_CASE("ThreadSafeObjectPool thread safety", "[objectpool][threadsafe]")
{
    TestObject::resetStats();

    {
        // Create a simplified test for thread safety
        // Just allocate and free objects from multiple threads without
        // tracking them in thread-local collections
        ThreadSafeObjectPool<TestObject> pool;
        std::atomic<int> allocCount{0};

        constexpr int numThreads = 4;
        constexpr int allocsPerThread = 25;

        std::vector<std::thread> threads;

        // Create threads that allocate objects
        for (int t = 0; t < numThreads; t++)
        {
            threads.emplace_back(
                [&pool, &allocCount, t]()
                {
                    for (int i = 0; i < allocsPerThread; i++)
                    {
                        int value = t * 1000 + i;
                        TestObject* obj = pool.allocate(value);
                        if (obj != nullptr && obj->getValue() == value)
                        {
                            allocCount.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                });
        }

        // Join allocation threads
        for (auto& thread : threads)
        {
            thread.join();
        }

        // Verify all allocations succeeded
        REQUIRE(allocCount.load() == numThreads * allocsPerThread);

        // Wait for allocations to complete
        REQUIRE(pool.getAllocationCount() == static_cast<size_t>(numThreads * allocsPerThread));

        // Clear the pool to free all objects
        pool.clear();

        // Verify all objects were freed
        REQUIRE(pool.getAllocationCount() == 0);
    }

    // Ensure all allocations were freed
    REQUIRE(TestObject::getConstructCount() == TestObject::getDestructCount());
}

// ThreadSafePolymorphicObjectPool Tests
TEST_CASE("ThreadSafePolymorphicObjectPool thread safety", "[polyobjectpool][threadsafe]")
{
    BaseClass::resetStats();

    {
        // Simplified thread safety test for polymorphic pool
        ThreadSafePolymorphicObjectPool<BaseClass> pool;
        std::atomic<int> allocCount{0};

        constexpr int numThreads = 4;
        constexpr int allocsPerThread = 25;

        std::vector<std::thread> threads;

        // Create threads that allocate objects
        for (int t = 0; t < numThreads; t++)
        {
            threads.emplace_back(
                [&pool, &allocCount, t]()
                {
                    for (int i = 0; i < allocsPerThread; i++)
                    {
                        // Alternate between derived types
                        if (i % 2 == 0)
                        {
                            DerivedClassA* obj = pool.allocate<DerivedClassA>(t * 1000 + i);
                            if (obj != nullptr && obj->getType() == 1)
                            {
                                allocCount.fetch_add(1, std::memory_order_relaxed);
                            }
                        }
                        else
                        {
                            DerivedClassB* obj = pool.allocate<DerivedClassB>(static_cast<float>(t * 1000 + i) * 0.5f);
                            if (obj != nullptr && obj->getType() == 2)
                            {
                                allocCount.fetch_add(1, std::memory_order_relaxed);
                            }
                        }
                    }
                });
        }

        // Join allocation threads
        for (auto& thread : threads)
        {
            thread.join();
        }

        // Verify all allocations succeeded
        REQUIRE(allocCount.load() == numThreads * allocsPerThread);
        REQUIRE(pool.getAllocationCount() == static_cast<size_t>(numThreads * allocsPerThread));

        // Clear the pool to free all objects
        pool.clear();

        // Verify all objects were freed
        REQUIRE(pool.getAllocationCount() == 0);
    }

    // Ensure all allocations were properly destroyed
    REQUIRE(BaseClass::getConstructCount() == BaseClass::getDestructCount());
}
