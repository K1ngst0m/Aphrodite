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

            value &= ~((TBitwise{ 1 } << one_count) - 1);
            bit_offset += one_count;
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

        size_t bit_offset = 0;
        while (true)
        {
            // Skip trailing zeroes
            size_t tz = 0;
            while (tz < value.size() && !value.test(tz))
            {
                ++tz;
            }
            if (tz >= value.size())
            {
                break; // no more set bits
            }
            bit_offset += tz;

            // Count the trailing ones
            size_t to = 0;
            while (to < value.size() && value.test(to))
            {
                ++to;
            }
            co_yield { static_cast<uint32_t>(bit_offset), static_cast<uint32_t>(to) };

            // Clear those "to" bits
            for (size_t i = 0; i < to; ++i)
            {
                value.reset(i);
            }
            bit_offset += to;
        }
    }
}
} // namespace aph::utils
