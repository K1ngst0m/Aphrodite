#include <catch2/catch_all.hpp>

#include "common/smallVector.h"

using namespace aph;

TEST_CASE("Default Constructor")
{
    SmallVector<int, 8> vec;
    REQUIRE(vec.empty());
    REQUIRE(vec.size() == 0);
    REQUIRE(vec.capacity() >= 8); // Assuming small buffer optimization
}

TEST_CASE("Fill Constructor")
{
    SmallVector<int, 8> vec(5, 42);

    REQUIRE(vec.size() == 5);
    REQUIRE(std::all_of(vec.begin(), vec.end(), [](int val) { return val == 42; }));
}

TEST_CASE("Initializer List Constructor")
{
    SmallVector<int, 8> vec = { 1, 2, 3, 4 };

    REQUIRE(vec.size() == 4);
    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 4);
}

TEST_CASE("Copy Constructor and Assignment")
{
    SmallVector<int, 8> original = { 1, 2, 3, 4 };
    SmallVector<int, 8> copy(original);

    REQUIRE(copy.size() == original.size());
    REQUIRE(std::equal(copy.begin(), copy.end(), original.begin()));

    SmallVector<int, 8> assigned;
    assigned = original;
    REQUIRE(assigned.size() == original.size());
    REQUIRE(std::equal(assigned.begin(), assigned.end(), original.begin()));
}

TEST_CASE("Move Constructor and Assignment")
{
    SmallVector<int, 8> original = { 1, 2, 3, 4 };
    SmallVector<int, 8> moved(std::move(original));

    REQUIRE(moved.size() == 4);
    REQUIRE(original.empty()); // Moved-from vector should be empty

    SmallVector<int, 8> assigned;
    assigned = std::move(moved);
    REQUIRE(assigned.size() == 4);
    REQUIRE(moved.empty()); // Moved-from vector should be empty
}

TEST_CASE("Allocator Behavior")
{
    SmallVector<int, 8> vec;
    vec.reserve(5); // Should use small buffer

    REQUIRE(vec.capacity() >= 8);
    REQUIRE(vec.size() == 0);

    vec.reserve(10); // Should switch to dynamic allocation
    REQUIRE(vec.capacity() >= 10);
}

TEST_CASE("Element Access")
{
    SmallVector<int, 8> vec = { 1, 2, 3, 4 };

    REQUIRE(vec[0] == 1);
    REQUIRE(vec.at(1) == 2);
    REQUIRE(vec.front() == 1);
    REQUIRE(vec.back() == 4);

    REQUIRE_THROWS_AS(vec.at(4), std::out_of_range);
}

TEST_CASE("Iterators")
{
    SmallVector<int, 8> vec = { 1, 2, 3, 4 };

    REQUIRE(*vec.begin() == 1);
    REQUIRE(*(vec.end() - 1) == 4);
    REQUIRE(*vec.rbegin() == 4);
    REQUIRE(*(vec.rend() - 1) == 1);
}
