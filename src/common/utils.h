#pragma once

#include <cassert>
#include <cmath>
#include <type_traits>

namespace aph::utils
{
constexpr uint32_t calculateFullMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

constexpr std::size_t paddingSize(std::size_t minAlignment, std::size_t originalSize)
{
    assert(minAlignment != 0 && "minAlignment must not be zero");
    assert((minAlignment & (minAlignment - 1)) == 0 && "minAlignment must be a power of two");
    return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
}

template <typename T>
std::underlying_type_t<T> getUnderlyingType(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}

} // namespace aph::utils

namespace aph
{
// Utility to handle dependent template failures
template <typename>
constexpr bool dependent_false_v = false;
} // namespace aph