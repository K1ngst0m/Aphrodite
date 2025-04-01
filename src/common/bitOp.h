#pragma once
#include "common/concept.h"
#include "coro/coro.hpp"

namespace aph::utils
{
template <typename T>
using Generator = coro::generator<T>;

template <BitwiseType T>
constexpr uint32_t leading_zeroes(T x) noexcept
{
    return std::countl_zero(x);
}

template <BitwiseType T>
constexpr uint32_t trailing_zeroes(T x) noexcept
{
    return std::countr_zero(x);
}

template <BitwiseType T>
constexpr uint32_t trailing_ones(T x) noexcept
{
    // ~x returns the bitwise complement, and counting its trailing zeroes gives the
    // number of consecutive 1's in x from the least-significant bit.
    return std::countr_zero(~x);
}

template <BitwiseType TBitwise>
Generator<uint32_t> forEachBit(TBitwise value) noexcept
{
    if constexpr (std::unsigned_integral<TBitwise>)
    {
        while (value)
        {
            uint32_t bit = trailing_zeroes(value);
            co_yield bit;
            value &= ~(TBitwise(1) << bit);
        }
    }
    else
    {
        while (value.any())
        {
            uint32_t bit = 0;
            while (!value.test(bit))
            {
                ++bit;
            }
            co_yield bit;
            value.reset(bit);
        }
    }
}

template <BitwiseType TBitwise>
Generator<std::pair<uint32_t, uint32_t>> forEachBitRange(TBitwise value) noexcept
{
    if constexpr (std::unsigned_integral<TBitwise>)
    {
        if (value == std::numeric_limits<TBitwise>::max())
        {
            co_yield { 0, static_cast<uint32_t>(sizeof(TBitwise) * 8) };
            co_return;
        }

        uint32_t bit_offset = 0;
        while (value != 0)
        {
            const uint32_t zero_count = trailing_zeroes(value);
            bit_offset += zero_count;
            value >>= zero_count;

            const uint32_t one_count = trailing_ones(value);
            co_yield { bit_offset, one_count };

            bit_offset += one_count;
            value >>= one_count;
        }
    }
    else
    {
        // Bitset path
        if (value.all())
        {
            co_yield { 0, static_cast<uint32_t>(value.size()) };
            co_return;
        }

        uint32_t bit_offset = 0;
        while (bit_offset < value.size())
        {
            // Skip zeroes
            while (bit_offset < value.size() && !value.test(bit_offset))
            {
                ++bit_offset;
            }

            if (bit_offset >= value.size())
            {
                break; // no more set bits
            }

            // Start of a range of ones
            uint32_t start_pos = bit_offset;

            // Count consecutive ones
            uint32_t length = 0;
            while (bit_offset < value.size() && value.test(bit_offset))
            {
                ++bit_offset;
                ++length;
            }

            co_yield { start_pos, length };
        }
    }
}
} // namespace aph::utils
