#include <catch2/catch_all.hpp>
#include "common/common.h"

using namespace aph::utils;
using namespace Catch;

// Assuming the forEachBit and forEachBitRange functions are defined here or included from another file

TEST_CASE("forEachBit: Test 1", "[forEachBit]")
{
    uint32_t              value    = 0b10100101;
    std::vector<uint32_t> expected = {0, 2, 5, 7};
    std::vector<uint32_t> result;

    forEachBit(value, [&](uint32_t bit_position) { result.push_back(bit_position); });

    REQUIRE(result == expected);
}

TEST_CASE("forEachBit: Test 2", "[forEachBit]")
{
    uint32_t              value    = 0b11110000;
    std::vector<uint32_t> expected = {4, 5, 6, 7};
    std::vector<uint32_t> result;

    forEachBit(value, [&](uint32_t bit_position) { result.push_back(bit_position); });

    REQUIRE(result == expected);
}

TEST_CASE("forEachBitRange: Test 1", "[forEachBitRange]")
{
    uint32_t                                   value    = 0b11011001;
    std::vector<std::pair<uint32_t, uint32_t>> expected = {{0, 1}, {3, 2}, {6, 2}};
    std::vector<std::pair<uint32_t, uint32_t>> result;

    forEachBitRange(value,
                    [&](uint32_t start_position, uint32_t length) { result.emplace_back(start_position, length); });

    REQUIRE(result == expected);
}

TEST_CASE("forEachBitRange: Test 2", "[forEachBitRange]")
{
    uint32_t                                   value    = 0b11110000;
    std::vector<std::pair<uint32_t, uint32_t>> expected = {{4, 4}};
    std::vector<std::pair<uint32_t, uint32_t>> result;

    forEachBitRange(value,
                    [&](uint32_t start_position, uint32_t length) { result.emplace_back(start_position, length); });

    REQUIRE(result == expected);
}

TEST_CASE("forEachBitRange: Test 3", "[forEachBitRange]")
{
    uint32_t                                   value    = 0b11111111;
    std::vector<std::pair<uint32_t, uint32_t>> expected = {{0, 8}};
    std::vector<std::pair<uint32_t, uint32_t>> result;

    forEachBitRange(value,
                    [&](uint32_t start_position, uint32_t length) { result.emplace_back(start_position, length); });

    REQUIRE(result == expected);
}
