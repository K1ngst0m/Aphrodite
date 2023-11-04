#ifndef APH_ENUM_HELPER_H_
#define APH_ENUM_HELPER_H_

#include <type_traits>

namespace aph
{
template <typename enum_t>
struct enable_enum_bitwise_operators
{
    static constexpr bool value = false;
};

template <typename enum_t>
constexpr bool enable_enum_bitwise_operators_v = enable_enum_bitwise_operators<enum_t>::value;

template <typename enum_t>
constexpr auto operator|(const enum_t& lhs, const enum_t& rhs) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, enum_t>
{
    return static_cast<enum_t>(static_cast<std::underlying_type_t<enum_t>>(lhs) |
                               static_cast<std::underlying_type_t<enum_t>>(rhs));
}

template <typename enum_t>
constexpr auto operator|=(enum_t& lhs, const enum_t& rhs) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, void>
{
    lhs = lhs | rhs;
}

template <typename enum_t>
constexpr auto operator&(const enum_t& lhs, const enum_t& rhs) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, enum_t>
{
    return static_cast<enum_t>(static_cast<std::underlying_type_t<enum_t>>(lhs) &
                               static_cast<std::underlying_type_t<enum_t>>(rhs));
}

template <typename enum_t>
constexpr auto operator&=(enum_t& lhs, const enum_t& rhs) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, void>
{
    lhs = lhs & rhs;
}

template <typename enum_t>
constexpr auto operator^(const enum_t& lhs, const enum_t& rhs) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, enum_t>
{
    return static_cast<enum_t>(static_cast<std::underlying_type_t<enum_t>>(lhs) ^
                               static_cast<std::underlying_type_t<enum_t>>(rhs));
}

template <typename enum_t>
constexpr auto operator^=(enum_t& lhs, const enum_t& rhs) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, void>
{
    lhs = lhs ^ rhs;
}

template <typename enum_t>
constexpr auto operator~(const enum_t& val) ->
    typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, enum_t>
{
    return static_cast<enum_t>(~static_cast<std::underlying_type_t<enum_t>>(val));
}

template <typename enum_t>
constexpr auto operator!(const enum_t& val) -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>, bool>
{
    return static_cast<std::underlying_type_t<enum_t>>(val) == static_cast<std::underlying_type_t<enum_t>>(0);
}

#define ENABLE_ENUM_BITWISE_OPERATORS(enum_t) \
    template <> \
    struct enable_enum_bitwise_operators<enum_t> \
    { \
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum."); \
        static constexpr bool value = true; \
    }
}  // namespace aph

#endif  // ENUMHELPER_H_
