#include "common/timer.h"
#include <catch2/catch_all.hpp>

using namespace aph;

TEST_CASE("Timer - Setting and getting intervals", "[Timer]")
{
    Timer timer;

    SECTION("Set and calculate interval")
    {
        timer.set("start");
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for a short duration
        timer.set("end");

        double interval = timer.interval("start", "end");
        REQUIRE(interval > 0.09);
        REQUIRE(interval < 0.11); // Should be close to 0.1
    }

    SECTION("Unknown tags should return zero and log an error")
    {
        double interval = timer.interval("unknown1", "unknown2");
        REQUIRE(interval == Catch::Approx(0.0)); // Using Approx for floating point comparison
    }
}

TEST_CASE("Timer - Timestamps are stored correctly", "[Timer]")
{
    Timer timer;
    timer.set("start");

    SECTION("Setting the same tag multiple times should overwrite the previous value")
    {
        timer.set("timestamp");
        auto firstInterval = timer.interval("start", "timestamp");

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sleep for a short duration
        timer.set("timestamp");
        auto secondInterval = timer.interval("start", "timestamp");

        REQUIRE(secondInterval > firstInterval);
    }
}
