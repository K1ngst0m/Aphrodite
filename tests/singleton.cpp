#include "common/singleton.h"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <thread>

using namespace aph;

class MySingleton : public Singleton<MySingleton>
{
    friend class Singleton<MySingleton>;

public:
    void setValue(int v)
    {
        value = v;
    }
    int getValue() const
    {
        return value;
    }

private:
    MySingleton() = default;
    int value{};
};

TEST_CASE("Basic Usage of Singleton", "[Singleton]")
{
    SECTION("Ensure we can create an instance")
    {
        MySingleton& instance1 = MySingleton::GetInstance();
        instance1.setValue(10);
        REQUIRE(instance1.getValue() == 10);
    }

    SECTION("Ensure repeated calls return same instance")
    {
        MySingleton& instance1 = MySingleton::GetInstance();
        MySingleton& instance2 = MySingleton::GetInstance();
        REQUIRE(&instance1 == &instance2); // Compare addresses
    }
}

TEST_CASE("Thread Safety of Singleton", "[Singleton]")
{
    std::vector<std::thread> threads;
    std::vector<MySingleton*> addresses;

    // Mutex to ensure synchronized access to the addresses vector
    std::mutex mtx;

    const int numThreads = 100;

    for (int i = 0; i < numThreads; ++i)
    {
        threads.push_back(std::thread(
            [&]()
            {
                MySingleton& instance = MySingleton::GetInstance();
                std::lock_guard<std::mutex> lock(mtx);
                addresses.push_back(&instance);
            }));
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    SECTION("All threads get the same instance")
    {
        for (std::size_t i = 1; i < addresses.size(); ++i)
        {
            REQUIRE(addresses[i] == addresses[0]);
        }
    }
}
