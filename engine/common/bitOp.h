#pragma once

namespace aph::utils
{
template <class T>
concept BitwiseType = std::unsigned_integral<T> || requires(const T& x) {
    { x.size() } -> std::convertible_to<std::size_t>;
    { x.test(0) } -> std::convertible_to<bool>;
};

// For 32-bit values
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

template <BitwiseType TBitwise, typename TFunc>
inline void forEachBit(TBitwise value, const TFunc& func) noexcept
{
    if constexpr (std::unsigned_integral<TBitwise>)
    {
        while (value)
        {
            uint32_t bit = trailing_zeroes(value);
            func(bit);
            value &= ~(1 << bit);
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
            func(bit);
            value.reset(bit);
        }
    }
}

template <BitwiseType TBitwise, typename TFunc>
inline void forEachBitRange(TBitwise value, const TFunc& func) noexcept
{
    if constexpr (std::unsigned_integral<TBitwise>)
    {
        if (value == std::numeric_limits<TBitwise>::max())
        {
            func(0, static_cast<uint32_t>(sizeof(TBitwise) * 8));
            return;
        }

        uint32_t bit_offset = 0;
        while (value != 0)
        {
            const uint32_t zero_count = trailing_zeroes(value);
            bit_offset += zero_count;
            value >>= zero_count;

            const uint32_t one_count = trailing_ones(value);
            func(bit_offset, one_count);

            value &= ~((TBitwise{ 1 } << one_count) - 1);
            bit_offset += one_count;
        }
    }
    else
    {
        // Bitset path
        // We'll simulate the same logic:
        if (value.all()) // If all bits in the bitset are set
        {
            func(0, static_cast<uint32_t>(value.size()));
            return;
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

            // Shift x right by tz bits
            // A direct shift of std::bitset<> is awkward if tz is not constant,
            // so we do it manually for demonstration:
            for (size_t i = 0; i + tz < value.size(); ++i)
            {
                value[i] = value[i + tz];
            }
            for (size_t i = value.size() - tz; i < value.size(); ++i)
            {
                value.reset(i);
            }

            // Now count the trailing ones
            size_t to = 0;
            while (to < value.size() && value.test(to))
            {
                ++to;
            }
            func(static_cast<uint32_t>(bit_offset), static_cast<uint32_t>(to));

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
