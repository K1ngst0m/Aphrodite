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

    APH_ALWAYS_INLINE bool success() const noexcept
    {
        return m_code == Code::Success;
    };

    APH_ALWAYS_INLINE Result(Code code, std::string_view msg = "")
        : m_code(code)
    {
        if (!msg.empty())
            m_msg = std::string(msg);
    }

    Code getCode() const noexcept
    {
        return m_code;
    }

    APH_ALWAYS_INLINE std::string_view toString() const noexcept
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

    operator std::string_view() const noexcept
    {
        return toString();
    }

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return success();
    }

private:
    Code m_code = Success;
    std::string m_msg = {};
};

struct [[nodiscard("Result should be handled.")]] ResultGroup
{
    APH_ALWAYS_INLINE ResultGroup() = default;

    APH_ALWAYS_INLINE ResultGroup(const Result& result)
    {
        append(result);
    }

    APH_ALWAYS_INLINE ResultGroup(Result&& result)
    {
        append(std::move(result));
    }

    APH_ALWAYS_INLINE ResultGroup(Result::Code code, std::string_view msg = "")
    {
        append(code, msg);
    }

    APH_ALWAYS_INLINE void append(Result::Code code, std::string_view msg = "")
    {
        if (code != Result::Success)
            m_hasFailure = true;

        m_results.emplace_back(code, msg);
    }

    APH_ALWAYS_INLINE void append(Result&& result)
    {
        if (!result.success())
            m_hasFailure = true;

        m_results.push_back(std::move(result));
    }

    APH_ALWAYS_INLINE void append(const Result& result)
    {
        if (!result.success())
            m_hasFailure = true;

        m_results.push_back(result);
    }

    template <typename T>
    APH_ALWAYS_INLINE ResultGroup& operator+=(T&& result)
    {
        append(std::forward<T>(result));
        return *this;
    }

    APH_ALWAYS_INLINE bool success() const noexcept
    {
        return !m_hasFailure;
    }

    APH_ALWAYS_INLINE operator Result() const noexcept
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

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return success();
    }

private:
    SmallVector<Result> m_results;
    bool m_hasFailure = false;
};

#ifdef APH_DEBUG
inline void APH_VERIFY_RESULT(Result result, const std::source_location source = std::source_location::current())
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

        APH_ALWAYS_INLINE std::string_view toString() const noexcept
        {
            return message;
        }

        APH_ALWAYS_INLINE static std::string defaultMessage(Code code) noexcept
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
        T value;
        Error error;

        APH_ALWAYS_INLINE Storage()
        {
        }
        APH_ALWAYS_INLINE ~Storage()
        {
        }

        template <typename... Args>
        APH_ALWAYS_INLINE void constructValue(Args&&... args)
        {
            new (&value) T(std::forward<Args>(args)...);
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
            m_storage.value.~T();
        }
        else
        {
            m_storage.error.~Error();
        }
    }

public:
    APH_ALWAYS_INLINE Expected() noexcept(std::is_nothrow_default_constructible_v<T>)
        : m_hasValue(true)
    {
        m_storage.constructValue();
    }

    // Value constructor
    template <typename U = T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, Expected>>>
    APH_ALWAYS_INLINE Expected(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
        : m_hasValue(true)
    {
        m_storage.constructValue(std::forward<U>(value));
    }

    // Error constructors
    APH_ALWAYS_INLINE Expected(Code code, std::string_view msg = "") noexcept
        : m_hasValue(false)
    {
        m_storage.constructError(code, msg);
    }

    // Error constructors
    APH_ALWAYS_INLINE Expected(Result result, std::string_view msg = "") noexcept
        : m_hasValue(false)
    {
        m_storage.constructError(result.getCode(), msg);
    }

    APH_ALWAYS_INLINE Expected(Error error) noexcept
        : m_hasValue(false)
    {
        m_storage.constructError(error.code, error.message);
    }

    // Copy and move constructors
    APH_ALWAYS_INLINE Expected(const Expected& other)
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

    APH_ALWAYS_INLINE Expected(Expected&& other) noexcept
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

    // Assignment operators
    APH_ALWAYS_INLINE Expected& operator=(const Expected& other)
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

    APH_ALWAYS_INLINE Expected& operator=(Expected&& other) noexcept
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

    // Destructor
    APH_ALWAYS_INLINE ~Expected() noexcept
    {
        destroy();
    }

    // Accessors
    APH_ALWAYS_INLINE bool hasValue() const noexcept
    {
        return m_hasValue;
    }

    APH_ALWAYS_INLINE bool success() const noexcept
    {
        return m_hasValue;
    }

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return m_hasValue;
    }

    APH_ALWAYS_INLINE operator T() noexcept
    {
        assert(success());
        return value();
    }

    // Value access - will assert if no value exists
    APH_ALWAYS_INLINE T& value() & noexcept
    {
        assert(m_hasValue && "Attempted to access value when Expected contains an error");
        return m_storage.value;
    }

    APH_ALWAYS_INLINE const T& value() const& noexcept
    {
        assert(m_hasValue && "Attempted to access value when Expected contains an error");
        return m_storage.value;
    }

    APH_ALWAYS_INLINE T&& value() && noexcept
    {
        assert(m_hasValue && "Attempted to access value when Expected contains an error");
        return std::move(m_storage.value);
    }

    // Value access with default
    template <typename U>
    APH_ALWAYS_INLINE T valueOr(U&& defaultValue) const&
    {
        return m_hasValue ? m_storage.value : static_cast<T>(std::forward<U>(defaultValue));
    }

    template <typename U>
    APH_ALWAYS_INLINE T valueOr(U&& defaultValue) &&
    {
        return m_hasValue ? std::move(m_storage.value) : static_cast<T>(std::forward<U>(defaultValue));
    }

    // Error access - will assert if value exists
    APH_ALWAYS_INLINE const Error& error() const& noexcept
    {
        assert(!m_hasValue && "Attempted to access error when Expected contains a value");
        return m_storage.error;
    }

    // Monadic operations
    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) const& -> decltype(f(std::declval<const T&>()))
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

    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) && -> decltype(f(std::declval<T&&>()))
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

    template <typename F>
    APH_ALWAYS_INLINE auto transform(F&& f) const& -> Expected<decltype(f(std::declval<const T&>()))>
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

    template <typename F>
    APH_ALWAYS_INLINE auto transform(F&& f) && -> Expected<decltype(f(std::declval<T&&>()))>
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

    template <typename F>
    APH_ALWAYS_INLINE auto or_else(F&& f) const& -> Expected<T>
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

    template <typename F>
    APH_ALWAYS_INLINE auto or_else(F&& f) && -> Expected<T>
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

    // Conversion to legacy Result
    APH_ALWAYS_INLINE operator Result() const noexcept
    {
        if (m_hasValue)
        {
            return Result(Result::Success);
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
};

// Deduction guide for Expected
template <typename T>
Expected(T) -> Expected<T>;

// Void specialization for Expected
template <>
struct [[nodiscard("Expected result should be handled")]] Expected<void>
{
    // Same as regular Expected but without value storage
    using Code = Expected<int>::Code;
    using Error = Expected<int>::Error;

private:
    std::optional<Error> m_error;

public:
    // Default constructor - success state for void
    APH_ALWAYS_INLINE Expected() noexcept
        : m_error(std::nullopt)
    {
    }

    // Error constructors
    APH_ALWAYS_INLINE Expected(Code code, std::string_view msg = "") noexcept
        : m_error(Error(code, msg))
    {
    }

    APH_ALWAYS_INLINE Expected(Error error) noexcept
        : m_error(std::move(error))
    {
    }

    // Accessors
    APH_ALWAYS_INLINE bool hasValue() const noexcept
    {
        return !m_error.has_value();
    }

    APH_ALWAYS_INLINE bool success() const noexcept
    {
        return !m_error.has_value();
    }

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return !m_error.has_value();
    }

    // Error access
    APH_ALWAYS_INLINE const Error& error() const& noexcept
    {
        assert(m_error.has_value() && "Attempted to access error when Expected is successful");
        return *m_error;
    }

    // Monadic operations
    template <typename F>
    APH_ALWAYS_INLINE auto and_then(F&& f) const -> decltype(f())
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

    template <typename F, typename R = std::invoke_result_t<F>>
    APH_ALWAYS_INLINE auto transform(F&& f) const -> Expected<R>
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
    APH_ALWAYS_INLINE Expected<void> or_else(F&& f) const
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

    // Conversion to legacy Result
    APH_ALWAYS_INLINE operator Result() const noexcept
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
};

// Enhanced ResultGroup that works with Expected
struct [[nodiscard("ResultGroup should be handled.")]] ExpectedGroup
{
    APH_ALWAYS_INLINE ExpectedGroup() = default;

    // Add a Result to the group
    APH_ALWAYS_INLINE void append(const Result& result)
    {
        if (!result.success())
            m_hasFailure = true;

        m_results.push_back(result);
    }

    // Add an Expected to the group
    template <typename T>
    APH_ALWAYS_INLINE void append(const Expected<T>& expected)
    {
        if (!expected.success())
        {
            m_hasFailure = true;
            m_results.push_back(Result(expected));
        }
    }

    // Various overloads for appending
    APH_ALWAYS_INLINE void append(Result::Code code, std::string_view msg = "")
    {
        if (code != Result::Success)
            m_hasFailure = true;

        m_results.emplace_back(code, msg);
    }

    // Operator overloads
    template <typename T>
    APH_ALWAYS_INLINE ExpectedGroup& operator+=(T&& result)
    {
        append(std::forward<T>(result));
        return *this;
    }

    APH_ALWAYS_INLINE bool success() const noexcept
    {
        return !m_hasFailure;
    }

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return success();
    }

    // Convert to Result for compatibility
    APH_ALWAYS_INLINE operator Result() const noexcept
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

    // Convert to Expected<void>
    APH_ALWAYS_INLINE operator Expected<void>() const noexcept
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

private:
    SmallVector<Result> m_results;
    bool m_hasFailure = false;
};
} // namespace aph
