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

    operator std::string_view() const noexcept
    {
        return toString();
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
inline void APH_VR(Result result, const std::source_location source = std::source_location::current())
{
    if (!result.success())
    {
        VK_LOG_ERR("Fatal : VkResult is \"%s\" in function[%s], %s:%d", result.toString(), source.function_name(),
                   source.file_name(), source.line());
        std::abort();
    }
}
#else
inline void APH_VR(Result result)
{
    return;
}
#endif
} // namespace aph 