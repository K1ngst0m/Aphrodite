#pragma once

namespace aph::utils
{
// For 32-bit values
constexpr uint32_t leading_zeroes(uint32_t x) noexcept
{
    return std::countl_zero(x);
}

constexpr uint32_t trailing_zeroes(uint32_t x) noexcept
{
    return std::countr_zero(x);
}

constexpr uint32_t trailing_ones(uint32_t x) noexcept
{
    // ~x returns the bitwise complement, and counting its trailing zeroes gives the
    // number of consecutive 1's in x from the least-significant bit.
    return std::countr_zero(~x);
}

// For 64-bit values
constexpr uint32_t leading_zeroes64(uint64_t x) noexcept
{
    return std::countl_zero(x);
}

constexpr uint32_t trailing_zeroes64(uint64_t x) noexcept
{
    return std::countr_zero(x);
}

constexpr uint32_t trailing_ones64(uint64_t x) noexcept
{
    return std::countr_zero(~x);
}

template <typename T>
inline void forEachBit64(uint64_t value, const T& func) noexcept
{
    while(value)
    {
        uint32_t bit = trailing_zeroes64(value);
        func(bit);
        value &= ~(1ull << bit);
    }
}

template <typename T>
inline void forEachBit(uint32_t value, const T& func) noexcept
{
    while(value)
    {
        uint32_t bit = trailing_zeroes(value);
        func(bit);
        value &= ~(1u << bit);
    }
}

template <typename T>
inline void forEachBitRange(uint32_t value, const T& func) noexcept
{
    if(value == ~0u)
    {
        func(0, 32);
        return;
    }

    uint32_t bit_offset = 0;
    while(value)
    {
        uint32_t bit = trailing_zeroes(value);
        bit_offset += bit;
        value >>= bit;
        uint32_t range = trailing_ones(value);
        func(bit_offset, range);
        value &= ~((1u << range) - 1);
    }
}
}  // namespace aph::utils
