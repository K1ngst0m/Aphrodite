#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace aph
{

template <typename T>
struct FunctionTraits;

template <typename Return, typename... Args>
struct FunctionTraits<Return (*)(Args...)>
{
    using ReturnType = Return;
    using ArgumentTypes = std::tuple<Args...>;

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    using ArgumentType = std::tuple_element_t<N, ArgumentTypes>;
};

template <typename Return, typename Class, typename... Args>
struct FunctionTraits<Return (Class::*)(Args...)>
{
    using ReturnType = Return;
    using ClassType = Class;
    using ArgumentTypes = std::tuple<Args...>;

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    using ArgumentType = std::tuple_element_t<N, ArgumentTypes>;
};

template <typename Return, typename Class, typename... Args>
struct FunctionTraits<Return (Class::*)(Args...) const>
{
    using ReturnType = Return;
    using ClassType = Class;
    using ArgumentTypes = std::tuple<Args...>;

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    using ArgumentType = std::tuple_element_t<N, ArgumentTypes>;
};

template <typename Return, typename... Args>
struct FunctionTraits<Return (*)(Args...) noexcept> : FunctionTraits<Return (*)(Args...)>
{
};

template <typename Return, typename Class, typename... Args>
struct FunctionTraits<Return (Class::*)(Args...) noexcept> : FunctionTraits<Return (Class::*)(Args...)>
{
};

template <typename Return, typename Class, typename... Args>
struct FunctionTraits<Return (Class::*)(Args...) const noexcept> : FunctionTraits<Return (Class::*)(Args...) const>
{
};

template <typename Return, typename... Args>
struct FunctionTraits<std::function<Return(Args...)>> : FunctionTraits<Return (*)(Args...)>
{
};

template <typename Functor>
struct FunctionTraits : FunctionTraits<decltype(&Functor::operator())>
{
};

template <typename T>
struct FunctionTraits<std::reference_wrapper<T>> : FunctionTraits<T>
{
};

template <typename T>
struct FunctionTraits<T&> : FunctionTraits<T>
{
};

template <typename T>
struct FunctionTraits<T&&> : FunctionTraits<T>
{
};

template <typename T>
struct FunctionTraits<const T> : FunctionTraits<T>
{
};

template <typename T>
struct FunctionTraits<volatile T> : FunctionTraits<T>
{
};

template <typename T>
struct FunctionTraits<const volatile T> : FunctionTraits<T>
{
};

template <typename F>
using FunctionReturnType = typename FunctionTraits<F>::ReturnType;

template <typename F>
using FunctionArguments = typename FunctionTraits<F>::ArgumentTypes;

template <typename F, size_t N>
using FunctionArgumentType = typename FunctionTraits<F>::template ArgumentType<N>;

template <typename F>
inline constexpr size_t functionArity = FunctionTraits<F>::arity;

template <typename F, typename... Args>
inline constexpr bool isCallableWith = std::is_invocable_r_v<FunctionReturnType<F>, F, Args...>;
} // namespace aph
