#include "common/bitOp.h"

#include <catch2/catch_all.hpp>

using namespace aph::utils;
using namespace Catch;

// Assuming the forEachBit and forEachBitRange functions are defined here or included from another file

TEST_CASE("forEachBit: Test 1", "[forEachBit]")
{
    uint32_t value = 0b10100101;
    std::vector<uint32_t> expected = { 0, 2, 5, 7 };
    std::vector<uint32_t> result;

    for (auto bit_position : forEachBit(value))
    {
        result.push_back(bit_position);
    }

    REQUIRE(result == expected);
}

TEST_CASE("forEachBit: Test 2", "[forEachBit]")
{
    uint32_t value = 0b11110000;
    std::vector<uint32_t> expected = { 4, 5, 6, 7 };
    std::vector<uint32_t> result;

    for (auto bit_position : forEachBit(value)) 
    {
        result.push_back(bit_position);
    }

    REQUIRE(result == expected);
}

TEST_CASE("forEachBitRange: Test 1", "[forEachBitRange]")
{
    uint32_t value = 0b11011001;
    std::vector<std::pair<uint32_t, uint32_t>> expected = { { 0, 1 }, { 3, 2 }, { 6, 2 } };
    std::vector<std::pair<uint32_t, uint32_t>> result;

    for (auto [start_position, length] : forEachBitRange(value)) 
    {
        result.emplace_back(start_position, length);
        // Print values to help with debugging
        INFO("Found range: start=" << start_position << ", length=" << length);
    }

    // Compare each pair individually for better error messages
    REQUIRE(result.size() == expected.size());
    for (size_t i = 0; i < result.size() && i < expected.size(); ++i) {
        INFO("Comparing pair " << i);
        REQUIRE(result[i].first == expected[i].first);
        REQUIRE(result[i].second == expected[i].second);
    }
    
    // Also check the full vector
    REQUIRE(result == expected);
}

TEST_CASE("forEachBitRange: Test 2", "[forEachBitRange]")
{
    uint32_t value = 0b11110000;
    std::vector<std::pair<uint32_t, uint32_t>> expected = { { 4, 4 } };
    std::vector<std::pair<uint32_t, uint32_t>> result;

    for (auto [start_position, length] : forEachBitRange(value)) 
    {
        result.emplace_back(start_position, length);
    }

    REQUIRE(result == expected);
}

TEST_CASE("forEachBitRange: Test 3", "[forEachBitRange]")
{
    uint32_t value = 0b11111111;
    std::vector<std::pair<uint32_t, uint32_t>> expected = { { 0, 8 } };
    std::vector<std::pair<uint32_t, uint32_t>> result;

    for (auto [start_position, length] : forEachBitRange(value)) 
    {
        result.emplace_back(start_position, length);
    }

    REQUIRE(result == expected);
}
