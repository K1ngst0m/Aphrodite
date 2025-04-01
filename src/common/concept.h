#pragma once

namespace aph
{
template <typename T>
concept NumericType = std::integral<T> || std::floating_point<T>;

template <class T>
concept BitwiseType = std::unsigned_integral<T> || requires(const T& x) {
    { x.size() } -> std::convertible_to<std::size_t>;
    { x.test(0) } -> std::convertible_to<bool>;
};

template <typename T>
concept StringType = std::is_convertible_v<T, std::string_view>;
} // namespace aph
