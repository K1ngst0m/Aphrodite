#pragma once

#include "bitOp.h"
#include "logger.h"
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
APH_ALWAYS_INLINE void APH_ASSERT(T condition, const std::source_location& loc = std::source_location::current())
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

private:
    Code m_code = Success;
    std::string m_msg = {};
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
template <class T>
void hashCombine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
constexpr uint32_t calculateFullMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
template <typename T>
std::underlying_type_t<T> getUnderLyingType(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}
} // namespace aph::utils
