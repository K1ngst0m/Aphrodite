#pragma once

#include "debug.h"
#include "smallVector.h"
#include <string>
#include <string_view>

namespace aph
{
struct [[nodiscard("Result should be handled.")]] Result
{
    enum Code
    {
        Success,
        ArgumentOutOfRange,
        RuntimeError,
    };

    APH_ALWAYS_INLINE auto success() const noexcept -> bool;
    APH_ALWAYS_INLINE operator bool() const noexcept;
    APH_ALWAYS_INLINE auto toString() const noexcept -> std::string_view;
    APH_ALWAYS_INLINE operator std::string_view() const noexcept;
    APH_ALWAYS_INLINE auto getCode() const noexcept -> Code;

    APH_ALWAYS_INLINE Result(Code code, std::string_view msg = "");

private:
    Code m_code       = Success;
    std::string m_msg = {};
};

struct [[nodiscard("Result should be handled.")]] ResultGroup
{
    APH_ALWAYS_INLINE auto success() const noexcept -> bool;
    APH_ALWAYS_INLINE operator bool() const noexcept;
    APH_ALWAYS_INLINE operator Result() const noexcept;

    APH_ALWAYS_INLINE ResultGroup() = default;
    APH_ALWAYS_INLINE ResultGroup(const Result& result);
    APH_ALWAYS_INLINE ResultGroup(Result&& result);
    APH_ALWAYS_INLINE ResultGroup(Result::Code code, std::string_view msg = "");

    APH_ALWAYS_INLINE void append(Result::Code code, std::string_view msg = "");
    APH_ALWAYS_INLINE void append(Result&& result);
    APH_ALWAYS_INLINE void append(const Result& result);
    template <typename T>
    APH_ALWAYS_INLINE auto operator+=(T&& result) -> ResultGroup&;

private:
    SmallVector<Result> m_results;
    bool m_hasFailure = false;
};

#ifdef APH_DEBUG
inline void APH_VERIFY_RESULT(const Result& result, const std::source_location& source)
{
    if (!result.success())
    {
        VK_LOG_ERR("Fatal : VkResult is \"%s\" in function[%s], %s:%d", result.toString(), source.function_name(),
                   source.file_name(), source.line());
        std::abort();
    }
}
#else
inline void APH_VERIFY_RESULT(Result result)
{
    return;
}
#endif

template <typename T>
struct [[nodiscard("Expected result should be handled")]] Expected
{
    using Code = Result::Code;

    struct Error
    {
        Code code;
        std::string message;

        APH_ALWAYS_INLINE Error(Code c, std::string_view msg = "");
        APH_ALWAYS_INLINE auto toString() const noexcept -> std::string_view;
        APH_ALWAYS_INLINE static auto defaultMessage(Code code) noexcept -> std::string;
    };

private:
    union Storage
    {
        T value;
        Error error;

        APH_ALWAYS_INLINE Storage();
        APH_ALWAYS_INLINE ~Storage();
        template <typename... Args>
        APH_ALWAYS_INLINE void constructValue(Args&&... args);
        APH_ALWAYS_INLINE void constructError(Code code, std::string_view msg = "");
    };

    Storage m_storage;
    bool m_hasValue = false;
    APH_ALWAYS_INLINE void destroy() noexcept;

public:
    APH_ALWAYS_INLINE auto hasValue() const noexcept -> bool;
    APH_ALWAYS_INLINE auto success() const noexcept -> bool;
    APH_ALWAYS_INLINE operator bool() const noexcept;
    APH_ALWAYS_INLINE operator T() noexcept;
    APH_ALWAYS_INLINE operator Result() const noexcept;

    APH_ALWAYS_INLINE auto value() & noexcept -> T&;
    APH_ALWAYS_INLINE auto value() const& noexcept -> const T&;
    APH_ALWAYS_INLINE auto value() && noexcept -> T&&;
    template <typename U>
    APH_ALWAYS_INLINE auto valueOr(U&& defaultValue) const& -> T;
    template <typename U>
    APH_ALWAYS_INLINE auto valueOr(U&& defaultValue) && -> T;

    APH_ALWAYS_INLINE auto error() const& noexcept -> const Error&;

    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) const& -> decltype(f(std::declval<const T&>()));
    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) && -> decltype(f(std::declval<T&&>()));
    template <typename F>
    APH_ALWAYS_INLINE auto transform(F&& f) const& -> Expected<decltype(f(std::declval<const T&>()))>;
    template <typename F>
    APH_ALWAYS_INLINE auto transform(F&& f) && -> Expected<decltype(f(std::declval<T&&>()))>;
    template <typename F>
    APH_ALWAYS_INLINE auto or_else(F&& f) const& -> Expected<T>;
    template <typename F>
    APH_ALWAYS_INLINE auto or_else(F&& f) && -> Expected<T>;

    APH_ALWAYS_INLINE Expected() noexcept(std::is_nothrow_default_constructible_v<T>);
    template <typename U = T>
    APH_ALWAYS_INLINE Expected(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
        requires (!std::is_same_v<std::decay_t<U>, Expected>);
    APH_ALWAYS_INLINE Expected(Code code, std::string_view msg = "") noexcept;
    APH_ALWAYS_INLINE Expected(const Result& result, std::string_view msg = "") noexcept;
    APH_ALWAYS_INLINE Expected(Error error) noexcept;
    APH_ALWAYS_INLINE Expected(const Expected& other);
    APH_ALWAYS_INLINE Expected(Expected&& other) noexcept;

    APH_ALWAYS_INLINE auto operator=(const Expected& other) -> Expected&;
    APH_ALWAYS_INLINE auto operator=(Expected&& other) noexcept -> Expected&;

    APH_ALWAYS_INLINE ~Expected() noexcept;
};

template <typename T>
Expected(T) -> Expected<T>;

template <>
struct [[nodiscard("Expected result should be handled")]] Expected<void>
{
    using Code  = Expected<int>::Code;
    using Error = Expected<int>::Error;

private:
    std::optional<Error> m_error;

public:
    APH_ALWAYS_INLINE auto hasValue() const noexcept -> bool;
    APH_ALWAYS_INLINE auto success() const noexcept -> bool;
    APH_ALWAYS_INLINE operator bool() const noexcept;
    APH_ALWAYS_INLINE auto error() const& noexcept -> const Error&;
    APH_ALWAYS_INLINE operator Result() const noexcept;

    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) const -> decltype(f());
    template <typename F, typename R = std::invoke_result_t<F>>
    APH_ALWAYS_INLINE auto transform(F&& f) const -> Expected<R>;
    template <typename F>
    APH_ALWAYS_INLINE auto or_else(F&& f) const -> Expected<void>;

    APH_ALWAYS_INLINE Expected() noexcept;
    APH_ALWAYS_INLINE Expected(Code code, std::string_view msg = "") noexcept;
    APH_ALWAYS_INLINE Expected(Error error) noexcept;
};

struct [[nodiscard("ResultGroup should be handled.")]] ExpectedGroup
{
    APH_ALWAYS_INLINE auto success() const noexcept -> bool;
    APH_ALWAYS_INLINE operator bool() const noexcept;
    APH_ALWAYS_INLINE operator Result() const noexcept;
    APH_ALWAYS_INLINE operator Expected<void>() const noexcept;

    APH_ALWAYS_INLINE ExpectedGroup() = default;

    APH_ALWAYS_INLINE void append(const Result& result);
    template <typename T>
    APH_ALWAYS_INLINE void append(const Expected<T>& expected);
    APH_ALWAYS_INLINE void append(Result::Code code, std::string_view msg = "");
    template <typename T>
    APH_ALWAYS_INLINE auto operator+=(T&& result) -> ExpectedGroup&;

private:
    SmallVector<Result> m_results;
    bool m_hasFailure = false;
};

// Result implementation
APH_ALWAYS_INLINE auto Result::success() const noexcept -> bool
{
    return m_code == Code::Success;
}

APH_ALWAYS_INLINE Result::Result(Code code, std::string_view msg)
    : m_code(code)
{
    if (!msg.empty())
        m_msg = std::string(msg);
#ifdef APH_DEBUG
    if (code != Code::Success)
    {
        VK_LOG_ERR("Fatal: Result failed with \"%s\"", msg.empty() ? toString().data() : msg.data());
        std::abort();
    }
#endif
}

APH_ALWAYS_INLINE auto Result::getCode() const noexcept -> Code
{
    return m_code;
}

APH_ALWAYS_INLINE auto Result::toString() const noexcept -> std::string_view
{
    if (!m_msg.empty())
    {
        return m_msg;
    }
    switch (m_code)
    {
    case Success:
        return "Success.";
    case ArgumentOutOfRange:
        return "Argument Out of Range.";
    case RuntimeError:
        return "Runtime Error.";
    default:
        return "Unknown";
    }
}

APH_ALWAYS_INLINE Result::operator std::string_view() const noexcept
{
    return toString();
}

APH_ALWAYS_INLINE Result::operator bool() const noexcept
{
    return success();
}

// ResultGroup implementation
APH_ALWAYS_INLINE ResultGroup::ResultGroup(const Result& result)
{
    append(result);
}

APH_ALWAYS_INLINE ResultGroup::ResultGroup(Result&& result)
{
    append(std::move(result));
}

APH_ALWAYS_INLINE ResultGroup::ResultGroup(Result::Code code, std::string_view msg)
{
    append(code, msg);
}

APH_ALWAYS_INLINE void ResultGroup::append(Result::Code code, std::string_view msg)
{
    if (code != Result::Success)
        m_hasFailure = true;

    m_results.emplace_back(code, msg);
}

APH_ALWAYS_INLINE void ResultGroup::append(Result&& result)
{
    if (!result.success())
        m_hasFailure = true;

    m_results.push_back(std::move(result));
}

APH_ALWAYS_INLINE void ResultGroup::append(const Result& result)
{
    if (!result.success())
        m_hasFailure = true;

    m_results.push_back(result);
}

template <typename T>
APH_ALWAYS_INLINE auto ResultGroup::operator+=(T&& result) -> ResultGroup&
{
    append(std::forward<T>(result));
    return *this;
}

APH_ALWAYS_INLINE auto ResultGroup::success() const noexcept -> bool
{
    return !m_hasFailure;
}

APH_ALWAYS_INLINE ResultGroup::operator Result() const noexcept
{
    if (success())
    {
        return Result::Success;
    }
    else
    {
        for (const auto& res : m_results)
        {
            if (!res.success())
                return res;
        }
        // Should never reach here if m_hasFailure is correctly maintained
        return Result::RuntimeError;
    }
}

APH_ALWAYS_INLINE ResultGroup::operator bool() const noexcept
{
    return success();
}

// Expected<T>::Error implementation
template <typename T>
APH_ALWAYS_INLINE Expected<T>::Error::Error(Code c, std::string_view msg)
    : code(c)
    , message(msg.empty() ? defaultMessage(c) : std::string(msg))
{
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::Error::toString() const noexcept -> std::string_view
{
    return message;
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::Error::defaultMessage(Code code) noexcept -> std::string
{
    switch (code)
    {
    case Code::Success:
        return "Success.";
    case Code::ArgumentOutOfRange:
        return "Argument Out of Range.";
    case Code::RuntimeError:
        return "Runtime Error.";
    default:
        return "Unknown Error.";
    }
}

// Expected<T>::Storage implementation
template <typename T>
APH_ALWAYS_INLINE Expected<T>::Storage::Storage()
{
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::Storage::~Storage()
{
}

template <typename T>
template <typename... Args>
APH_ALWAYS_INLINE void Expected<T>::Storage::constructValue(Args&&... args)
{
    new (&value) T(std::forward<Args>(args)...);
}

template <typename T>
APH_ALWAYS_INLINE void Expected<T>::Storage::constructError(Code code, std::string_view msg)
{
    new (&error) Error(code, msg);
}

// Expected<T> private method implementations
template <typename T>
APH_ALWAYS_INLINE void Expected<T>::destroy() noexcept
{
    if (m_hasValue)
    {
        m_storage.value.~T();
    }
    else
    {
        m_storage.error.~Error();
    }
}

// Expected<T> public method implementations
template <typename T>
APH_ALWAYS_INLINE Expected<T>::Expected() noexcept(std::is_nothrow_default_constructible_v<T>)
    : m_hasValue(true)
{
    m_storage.constructValue();
}

template <typename T>
template <typename U>
APH_ALWAYS_INLINE Expected<T>::Expected(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
    requires (!std::is_same_v<std::decay_t<U>, Expected>) : m_hasValue(true)
{
    m_storage.constructValue(std::forward<U>(value));
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::Expected(Code code, std::string_view msg) noexcept
    : m_hasValue(false)
{
    m_storage.constructError(code, msg);
#ifdef APH_DEBUG
    if (code != Code::Success)
    {
        VK_LOG_ERR("Fatal: Expected failed with \"%s\"", msg.empty() ? Error::defaultMessage(code).c_str() : std::string(msg).c_str());
        std::abort();
    }
#endif
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::Expected(const Result& result, std::string_view msg) noexcept
    : m_hasValue(false)
{
    m_storage.constructError(result.getCode(), msg);
#ifdef APH_DEBUG
    if (!result.success())
    {
        std::string errorMsg = msg.empty() ? std::string(result.toString()) : std::string(msg);
        VK_LOG_ERR("Fatal: Expected failed with \"%s\"", errorMsg.c_str());
        std::abort();
    }
#endif
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::Expected(Error error) noexcept
    : m_hasValue(false)
{
    m_storage.constructError(error.code, error.message);
#ifdef APH_DEBUG
    if (error.code != Code::Success)
    {
        VK_LOG_ERR("Fatal: Expected failed with \"%s\"", error.message.c_str());
        std::abort();
    }
#endif
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::Expected(const Expected& other)
    : m_hasValue(other.m_hasValue)
{
    if (m_hasValue)
    {
        m_storage.constructValue(other.m_storage.value);
    }
    else
    {
        m_storage.constructError(other.m_storage.error.code, other.m_storage.error.message);
    }
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::Expected(Expected&& other) noexcept
    : m_hasValue(other.m_hasValue)
{
    if (m_hasValue)
    {
        m_storage.constructValue(std::move(other.m_storage.value));
    }
    else
    {
        m_storage.constructError(other.m_storage.error.code, other.m_storage.error.message);
    }
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::operator=(const Expected& other) -> Expected&
{
    if (this != &other)
    {
        destroy();
        m_hasValue = other.m_hasValue;

        if (m_hasValue)
        {
            m_storage.constructValue(other.m_storage.value);
        }
        else
        {
            m_storage.constructError(other.m_storage.error.code, other.m_storage.error.message);
        }
    }
    return *this;
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::operator=(Expected&& other) noexcept -> Expected&
{
    if (this != &other)
    {
        destroy();
        m_hasValue = other.m_hasValue;

        if (m_hasValue)
        {
            m_storage.constructValue(std::move(other.m_storage.value));
        }
        else
        {
            m_storage.constructError(other.m_storage.error.code, std::move(other.m_storage.error.message));
        }
    }
    return *this;
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::~Expected() noexcept
{
    destroy();
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::hasValue() const noexcept -> bool
{
    return m_hasValue;
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::success() const noexcept -> bool
{
    return m_hasValue;
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::operator bool() const noexcept
{
    return m_hasValue;
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::operator T() noexcept
{
    return value();
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::value() & noexcept -> T&
{
    assert(m_hasValue && "Attempted to access value when Expected contains an error");
    return m_storage.value;
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::value() const& noexcept -> const T&
{
    assert(m_hasValue && "Attempted to access value when Expected contains an error");
    return m_storage.value;
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::value() && noexcept -> T&&
{
    assert(m_hasValue && "Attempted to access value when Expected contains an error");
    return std::move(m_storage.value);
}

template <typename T>
template <typename U>
APH_ALWAYS_INLINE auto Expected<T>::valueOr(U&& defaultValue) const& -> T
{
    return m_hasValue ? m_storage.value : static_cast<T>(std::forward<U>(defaultValue));
}

template <typename T>
template <typename U>
APH_ALWAYS_INLINE auto Expected<T>::valueOr(U&& defaultValue) && -> T
{
    return m_hasValue ? std::move(m_storage.value) : static_cast<T>(std::forward<U>(defaultValue));
}

template <typename T>
APH_ALWAYS_INLINE auto Expected<T>::error() const& noexcept -> const Error&
{
    assert(!m_hasValue && "Attempted to access error when Expected contains a value");
    return m_storage.error;
}

template <typename T>
template <typename F>
APH_ALWAYS_INLINE auto Expected<T>::and_then(F&& f) const& -> decltype(f(std::declval<const T&>()))
{
    using ReturnType = decltype(f(std::declval<const T&>()));

    if (m_hasValue)
    {
        return f(m_storage.value);
    }
    else
    {
        return ReturnType(m_storage.error.code, m_storage.error.message);
    }
}

template <typename T>
template <typename F>
APH_ALWAYS_INLINE auto Expected<T>::and_then(F&& f) && -> decltype(f(std::declval<T&&>()))
{
    using ReturnType = decltype(f(std::declval<T&&>()));

    if (m_hasValue)
    {
        return f(std::move(m_storage.value));
    }
    else
    {
        return ReturnType(m_storage.error.code, m_storage.error.message);
    }
}

template <typename T>
template <typename F>
APH_ALWAYS_INLINE auto Expected<T>::transform(F&& f) const& -> Expected<decltype(f(std::declval<const T&>()))>
{
    using ReturnType = decltype(f(std::declval<const T&>()));

    if (m_hasValue)
    {
        return Expected<ReturnType>(f(m_storage.value));
    }
    else
    {
        return Expected<ReturnType>(m_storage.error.code, m_storage.error.message);
    }
}

template <typename T>
template <typename F>
APH_ALWAYS_INLINE auto Expected<T>::transform(F&& f) && -> Expected<decltype(f(std::declval<T&&>()))>
{
    using ReturnType = decltype(f(std::declval<T&&>()));

    if (m_hasValue)
    {
        return Expected<ReturnType>(f(std::move(m_storage.value)));
    }
    else
    {
        return Expected<ReturnType>(m_storage.error.code, m_storage.error.message);
    }
}

template <typename T>
template <typename F>
APH_ALWAYS_INLINE auto Expected<T>::or_else(F&& f) const& -> Expected<T>
{
    if (!m_hasValue)
    {
        return f(m_storage.error);
    }
    else
    {
        return *this;
    }
}

template <typename T>
template <typename F>
APH_ALWAYS_INLINE auto Expected<T>::or_else(F&& f) && -> Expected<T>
{
    if (!m_hasValue)
    {
        return f(m_storage.error);
    }
    else
    {
        return std::move(*this);
    }
}

template <typename T>
APH_ALWAYS_INLINE Expected<T>::operator Result() const noexcept
{
    if (m_hasValue)
    {
        return Result::Success;
    }
    else
    {
        // Map the Expected::Code to Result::Code
        Result::Code resultCode;
        switch (m_storage.error.code)
        {
        case Code::ArgumentOutOfRange:
            resultCode = Result::ArgumentOutOfRange;
            break;
        case Code::RuntimeError:
            resultCode = Result::RuntimeError;
            break;
        default:
            resultCode = Result::RuntimeError;
            break;
        }
        return {resultCode, m_storage.error.message};
    }
}

// Expected<void> implementation
APH_ALWAYS_INLINE Expected<void>::Expected() noexcept
    : m_error(std::nullopt)
{
}

APH_ALWAYS_INLINE Expected<void>::Expected(Code code, std::string_view msg) noexcept
    : m_error(Error(code, msg))
{
#ifdef APH_DEBUG
    if (code != Code::Success)
    {
        VK_LOG_ERR("Fatal: Expected<void> failed with \"%s\"", msg.empty() ? Error::defaultMessage(code).c_str() : std::string(msg).c_str());
        std::abort();
    }
#endif
}

APH_ALWAYS_INLINE Expected<void>::Expected(Error error) noexcept
    : m_error(std::move(error))
{
#ifdef APH_DEBUG
    if (error.code != Code::Success)
    {
        VK_LOG_ERR("Fatal: Expected<void> failed with \"%s\"", error.message.c_str());
        std::abort();
    }
#endif
}

APH_ALWAYS_INLINE auto Expected<void>::hasValue() const noexcept -> bool
{
    return !m_error.has_value();
}

APH_ALWAYS_INLINE auto Expected<void>::success() const noexcept -> bool
{
    return !m_error.has_value();
}

APH_ALWAYS_INLINE Expected<void>::operator bool() const noexcept
{
    return !m_error.has_value();
}

APH_ALWAYS_INLINE auto Expected<void>::error() const& noexcept -> const Error&
{
    assert(m_error.has_value() && "Attempted to access error when Expected is successful");
    return *m_error;
}

template <typename F>
APH_ALWAYS_INLINE auto Expected<void>::and_then(F&& f) const -> decltype(f())
{
    using ReturnType = decltype(f());

    if (!m_error)
    {
        return f();
    }
    else
    {
        return ReturnType(m_error->code, m_error->message);
    }
}

template <typename F, typename R>
APH_ALWAYS_INLINE auto Expected<void>::transform(F&& f) const -> Expected<R>
{
    if (!m_error)
    {
        if constexpr (std::is_void_v<R>)
        {
            f();
            return Expected<void>();
        }
        else
        {
            return Expected<R>(f());
        }
    }
    else
    {
        return Expected<R>(m_error->code, m_error->message);
    }
}

template <typename F>
APH_ALWAYS_INLINE auto Expected<void>::or_else(F&& f) const -> Expected<void>
{
    if (m_error)
    {
        return f(*m_error);
    }
    else
    {
        return *this;
    }
}

APH_ALWAYS_INLINE Expected<void>::operator Result() const noexcept
{
    if (!m_error)
    {
        return Result(Result::Success);
    }
    else
    {
        Result::Code resultCode;
        switch (m_error->code)
        {
        case Code::ArgumentOutOfRange:
            resultCode = Result::ArgumentOutOfRange;
            break;
        case Code::RuntimeError:
            resultCode = Result::RuntimeError;
            break;
        default:
            resultCode = Result::RuntimeError;
            break;
        }
        return Result(resultCode, m_error->message);
    }
}

// ExpectedGroup implementation
APH_ALWAYS_INLINE void ExpectedGroup::append(const Result& result)
{
    if (!result.success())
        m_hasFailure = true;

    m_results.push_back(result);
}

template <typename T>
APH_ALWAYS_INLINE void ExpectedGroup::append(const Expected<T>& expected)
{
    if (!expected.success())
    {
        m_hasFailure = true;
        m_results.push_back(Result(expected));
    }
}

APH_ALWAYS_INLINE void ExpectedGroup::append(Result::Code code, std::string_view msg)
{
    if (code != Result::Success)
        m_hasFailure = true;

    m_results.emplace_back(code, msg);
}

template <typename T>
APH_ALWAYS_INLINE auto ExpectedGroup::operator+=(T&& result) -> ExpectedGroup&
{
    append(std::forward<T>(result));
    return *this;
}

APH_ALWAYS_INLINE auto ExpectedGroup::success() const noexcept -> bool
{
    return !m_hasFailure;
}

APH_ALWAYS_INLINE ExpectedGroup::operator bool() const noexcept
{
    return success();
}

APH_ALWAYS_INLINE ExpectedGroup::operator Result() const noexcept
{
    if (success())
    {
        return Result::Success;
    }
    else
    {
        for (const auto& res : m_results)
        {
            if (!res.success())
                return res;
        }
        return Result::RuntimeError;
    }
}

APH_ALWAYS_INLINE ExpectedGroup::operator Expected<void>() const noexcept
{
    if (success())
    {
        return Expected<void>();
    }
    else
    {
        for (const auto& res : m_results)
        {
            if (!res.success())
            {
                // Map legacy Result code to Expected::Code
                Expected<void>::Code code;
                switch (res.getCode())
                { // Assuming a getCode method exists
                case Result::ArgumentOutOfRange:
                    code = Expected<void>::Code::ArgumentOutOfRange;
                    break;
                case Result::RuntimeError:
                    code = Expected<void>::Code::RuntimeError;
                    break;
                default:
                    code = Expected<void>::Code::RuntimeError;
                    break;
                }
                return Expected<void>(code, res.toString());
            }
        }
        return Expected<void>(Expected<void>::Code::RuntimeError, "Unknown error");
    }
}

// Specialization for pointer types to handle Result properly
template <typename T>
struct [[nodiscard("Expected result should be handled")]] Expected<T*> 
{
    using Code = Result::Code;

    // Helper struct to store error information
    struct Error
    {
        Code code;
        std::string message;

        APH_ALWAYS_INLINE Error(Code c, std::string_view msg = "")
            : code(c)
            , message(msg.empty() ? defaultMessage(c) : std::string(msg))
        {
        }

        APH_ALWAYS_INLINE auto toString() const noexcept -> std::string_view
        {
            return message;
        }

        APH_ALWAYS_INLINE static auto defaultMessage(Code code) noexcept -> std::string
        {
            switch (code)
            {
            case Code::Success:
                return "Success.";
            case Code::ArgumentOutOfRange:
                return "Argument Out of Range.";
            case Code::RuntimeError:
                return "Runtime Error.";
            default:
                return "Unknown Error.";
            }
        }
    };

private:
    // Union to store either value or error
    union Storage
    {
        T* value;
        Error error;

        APH_ALWAYS_INLINE Storage() {}
        APH_ALWAYS_INLINE ~Storage() {}

        template <typename... Args>
        APH_ALWAYS_INLINE void constructValue(Args&&... args)
        {
            new (&value) T*(std::forward<Args>(args)...);
        }

        APH_ALWAYS_INLINE void constructError(Code code, std::string_view msg = "")
        {
            new (&error) Error(code, msg);
        }
    };

    Storage m_storage;
    bool m_hasValue = false;

    // Destroy the contained object
    APH_ALWAYS_INLINE void destroy() noexcept
    {
        if (m_hasValue)
        {
            // No need to call destructor on pointer type
        }
        else
        {
            m_storage.error.~Error();
        }
    }

public:
    APH_ALWAYS_INLINE Expected() noexcept
        : m_hasValue(true)
    {
        m_storage.value = nullptr;
    }

    // Value constructor
    APH_ALWAYS_INLINE Expected(T* value) noexcept
        : m_hasValue(true)
    {
        m_storage.value = value;
    }

    // Add constructor to handle conversions from other Expected types, especially std::nullptr_t
    template <typename U>
    APH_ALWAYS_INLINE Expected(const Expected<U>& other) noexcept
        : m_hasValue(other.hasValue())
    {
        if (m_hasValue)
        {
            if constexpr (std::is_convertible_v<U, T*>)
            {
                m_storage.value = static_cast<T*>(other.value());
            }
            else if constexpr (std::is_same_v<U, std::nullptr_t>)
            {
                m_storage.value = nullptr;
            }
            else
            {
                static_assert(std::is_convertible_v<U, T*> || std::is_same_v<U, std::nullptr_t>,
                              "Cannot convert between these Expected types");
            }
        }
        else
        {
            m_storage.constructError(other.error().code, other.error().message);
        }
    }

    // Also add move constructor for Expected<U> conversion
    template <typename U>
    APH_ALWAYS_INLINE Expected(Expected<U>&& other) noexcept
        : m_hasValue(other.hasValue())
    {
        if (m_hasValue)
        {
            if constexpr (std::is_convertible_v<U, T*>)
            {
                m_storage.value = static_cast<T*>(other.value());
            }
            else if constexpr (std::is_same_v<U, std::nullptr_t>)
            {
                m_storage.value = nullptr;
            }
            else
            {
                static_assert(std::is_convertible_v<U, T*> || std::is_same_v<U, std::nullptr_t>,
                              "Cannot convert between these Expected types");
            }
        }
        else
        {
            m_storage.constructError(other.error().code, std::move(other.error().message));
        }
    }

    // Handle Result specifically for pointer types - this is the key addition
    APH_ALWAYS_INLINE Expected(const Result& result, std::string_view msg = "") noexcept
        : m_hasValue(false)
    {
        m_storage.constructError(result.getCode(), msg);
#ifdef APH_DEBUG
        if (!result.success())
        {
            std::string errorMsg = msg.empty() ? std::string(result.toString()) : std::string(msg);
            VK_LOG_ERR("Fatal: Expected failed with \"%s\"", errorMsg.c_str());
            std::abort();
        }
#endif
    }

    // Error constructors
    APH_ALWAYS_INLINE Expected(Code code, std::string_view msg = "") noexcept
        : m_hasValue(false)
    {
        m_storage.constructError(code, msg);
#ifdef APH_DEBUG
        if (code != Code::Success)
        {
            VK_LOG_ERR("Fatal: Expected failed with \"%s\"", msg.empty() ? Error::defaultMessage(code).c_str() : std::string(msg).c_str());
            std::abort();
        }
#endif
    }

    APH_ALWAYS_INLINE Expected(Error error) noexcept
        : m_hasValue(false)
    {
        m_storage.constructError(error.code, error.message);
#ifdef APH_DEBUG
        if (error.code != Code::Success)
        {
            VK_LOG_ERR("Fatal: Expected failed with \"%s\"", error.message.c_str());
            std::abort();
        }
#endif
    }

    // Copy and move constructors
    APH_ALWAYS_INLINE Expected(const Expected& other)
        : m_hasValue(other.m_hasValue)
    {
        if (m_hasValue)
        {
            m_storage.value = other.m_storage.value;
        }
        else
        {
            m_storage.constructError(other.m_storage.error.code, other.m_storage.error.message);
        }
    }

    APH_ALWAYS_INLINE Expected(Expected&& other) noexcept
        : m_hasValue(other.m_hasValue)
    {
        if (m_hasValue)
        {
            m_storage.value = other.m_storage.value;
            other.m_storage.value = nullptr;
        }
        else
        {
            m_storage.constructError(other.m_storage.error.code, other.m_storage.error.message);
        }
    }

    // Assignment operators
    APH_ALWAYS_INLINE auto operator=(const Expected& other) -> Expected&
    {
        if (this != &other)
        {
            destroy();
            m_hasValue = other.m_hasValue;

            if (m_hasValue)
            {
                m_storage.value = other.m_storage.value;
            }
            else
            {
                m_storage.constructError(other.m_storage.error.code, other.m_storage.error.message);
            }
        }
        return *this;
    }

    APH_ALWAYS_INLINE auto operator=(Expected&& other) noexcept -> Expected&
    {
        if (this != &other)
        {
            destroy();
            m_hasValue = other.m_hasValue;

            if (m_hasValue)
            {
                m_storage.value = other.m_storage.value;
                other.m_storage.value = nullptr;
            }
            else
            {
                m_storage.constructError(other.m_storage.error.code, std::move(other.m_storage.error.message));
            }
        }
        return *this;
    }

    // Destructor
    APH_ALWAYS_INLINE ~Expected() noexcept
    {
        destroy();
    }

    // Accessors
    APH_ALWAYS_INLINE auto hasValue() const noexcept -> bool
    {
        return m_hasValue;
    }

    APH_ALWAYS_INLINE auto success() const noexcept -> bool
    {
        return m_hasValue;
    }

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return m_hasValue;
    }

    APH_ALWAYS_INLINE operator T*() noexcept
    {
        return value();
    }

    // Value access - will assert if no value exists
    APH_ALWAYS_INLINE auto value() noexcept -> T*
    {
        assert(m_hasValue && "Attempted to access value when Expected contains an error");
        return m_storage.value;
    }

    APH_ALWAYS_INLINE auto value() const noexcept -> T* const
    {
        assert(m_hasValue && "Attempted to access value when Expected contains an error");
        return m_storage.value;
    }

    // Value access with default
    APH_ALWAYS_INLINE auto valueOr(T* defaultValue) const -> T*
    {
        return m_hasValue ? m_storage.value : defaultValue;
    }

    // Error access - will assert if value exists
    APH_ALWAYS_INLINE auto error() const& noexcept -> const Error&
    {
        assert(!m_hasValue && "Attempted to access error when Expected contains a value");
        return m_storage.error;
    }

    // Monadic operations
    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) const -> decltype(f(std::declval<T*>()))
    {
        using ReturnType = decltype(f(std::declval<T*>()));

        if (m_hasValue)
        {
            return f(m_storage.value);
        }
        else
        {
            return ReturnType(m_storage.error.code, m_storage.error.message);
        }
    }

    template <typename F>
    APH_ALWAYS_INLINE auto transform(F&& f) const -> Expected<decltype(f(std::declval<T*>()))>
    {
        using ReturnType = decltype(f(std::declval<T*>()));

        if (m_hasValue)
        {
            return Expected<ReturnType>(f(m_storage.value));
        }
        else
        {
            return Expected<ReturnType>(m_storage.error.code, m_storage.error.message);
        }
    }

    template <typename F>
    APH_ALWAYS_INLINE auto or_else(F&& f) const -> Expected<T*>
    {
        if (!m_hasValue)
        {
            return f(m_storage.error);
        }
        else
        {
            return *this;
        }
    }

    // Conversion to legacy Result
    APH_ALWAYS_INLINE operator Result() const noexcept
    {
        if (m_hasValue)
        {
            return Result::Success;
        }
        else
        {
            Result::Code resultCode;
            switch (m_storage.error.code)
            {
            case Code::ArgumentOutOfRange:
                resultCode = Result::ArgumentOutOfRange;
                break;
            case Code::RuntimeError:
                resultCode = Result::RuntimeError;
                break;
            default:
                resultCode = Result::RuntimeError;
                break;
            }
            return {resultCode, m_storage.error.message};
        }
    }
};
} // namespace aph
