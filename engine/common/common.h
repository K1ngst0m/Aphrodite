#pragma once

#include "bitOp.h"
#include "concept.h"
#include "logger.h"
#include "smallVector.h"
#include <cmath>
#include <source_location>

#define APH_CONCAT_IMPL(x, y) x##y
#define APH_MACRO_CONCAT(x, y) APH_CONCAT_IMPL(x, y)

namespace aph
{

#if defined(_MSC_VER)
#define APH_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define APH_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define APH_ALWAYS_INLINE __forceinline
#else
#define APH_ALWAYS_INLINE inline
#endif

#define APH_FWD(x) std::forward<decltype(x)>(x)
} // namespace aph

namespace backward
{
class SignalHandling;
}

#include <cassert>
#include <signal.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif
namespace aph
{

APH_ALWAYS_INLINE void DebugBreak()
{
#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__linux__) || defined(__APPLE__)
    raise(SIGTRAP);
#else
    assert(false && "Debugger break triggered");
#endif
}

#ifdef APH_DEBUG
template <typename T>
    requires requires(T t) {
        { static_cast<bool>(t) } -> std::same_as<bool>;
    }
APH_ALWAYS_INLINE void APH_ASSERT(const T& condition, const std::source_location& loc = std::source_location::current())
{
    if (!static_cast<bool>(condition))
    {
        CM_LOG_ERR("Error at %s:%d.", loc.file_name(), loc.line());
        LOG_FLUSH();
        ::aph::DebugBreak();
    }
}
#else
inline void APH_ASSERT(bool condition) {};
#endif

class TracedException : public std::runtime_error
{
public:
    TracedException()
        : std::runtime_error(_get_trace())
    {
    }

private:
    std::string _get_trace();
};
extern backward::SignalHandling sh;
} // namespace aph

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

    APH_ALWAYS_INLINE Result(Code code, std::string msg = "")
        : m_code(code)
        , m_msg(std::move(msg))
    {
    }

    operator std::string_view()
    {
        return toString();
    }

    APH_ALWAYS_INLINE std::string_view toString()
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
        }

        APH_ASSERT(false);
        return "Unknown";
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
    APH_ALWAYS_INLINE ResultGroup(auto&& result)
    {
        append(APH_FWD(result));
    }

    APH_ALWAYS_INLINE ResultGroup(Result::Code code, std::string msg = "")
    {
        append(code, msg);
    }

    APH_ALWAYS_INLINE void append(Result::Code code, std::string msg = "")
    {
        m_results.emplace_back(code, msg);
    }

    APH_ALWAYS_INLINE void append(Result&& result)
    {
        m_results.push_back(APH_FWD(result));
    }

    APH_ALWAYS_INLINE ResultGroup& operator+=(auto&& result)
    {
        append(APH_FWD(result));
        return *this;
    }

    APH_ALWAYS_INLINE bool success() const noexcept
    {
        return std::all_of(m_results.cbegin(), m_results.cend(), [](const Result& res) { return res.success(); });
    }

    APH_ALWAYS_INLINE operator Result()
    {
        if (success())
        {
            return Result::Success;
        }
        else
        {
            auto errorRes =
                std::find_if_not(m_results.cbegin(), m_results.cend(), [](const Result& res) { return res.success(); });
            return *errorRes;
        }
    }

    APH_ALWAYS_INLINE operator bool() const noexcept
    {
        return success();
    }

private:
    SmallVector<Result> m_results;
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
    return result;
}
#endif
} // namespace aph

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
