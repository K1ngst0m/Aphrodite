#pragma once

#include <cmath>
#include "logger.h"
#include "bitOp.h"

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

APH_ALWAYS_INLINE bool Assert(bool cond, const char* file, int line, const char* format, ...)
{
    if(!cond)
    {
    }
    return cond;
}

#ifdef APH_DEBUG
    #define APH_ASSERT(x) \
        do \
        { \
            if(!bool(x)) \
            { \
                CM_LOG_ERR("Error at %s:%d.", __FILE__, __LINE__); \
                LOG_FLUSH(); \
                aph::DebugBreak(); \
            } \
        } while(0)
#else
    #define APH_ASSERT(x) ((void)0)
#endif
}  // namespace aph

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

APH_ALWAYS_INLINE void DebugBreak() {
#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__linux__) || defined(__APPLE__)
    raise(SIGTRAP);
#else
    assert(false && "Debugger break triggered");
#endif
}

class TracedException : public std::runtime_error
{
public:
    TracedException() : std::runtime_error(_get_trace()) {}

private:
    std::string _get_trace();
};
extern backward::SignalHandling sh;
}  // namespace aph

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

    APH_ALWAYS_INLINE bool success() const { return m_code == Code::Success; };

    APH_ALWAYS_INLINE Result(Code code, std::string msg = "") : m_code(code), m_msg(std::move(msg)) {}

    APH_ALWAYS_INLINE std::string_view toString()
    {
        if(!m_msg.empty())
        {
            return m_msg;
        }
        switch(m_code)
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
    Code        m_code;
    std::string m_msg;
};

#define APH_VR(f) \
    { \
        ::aph::Result res = (f); \
        if(!res.success()) \
        { \
            CM_LOG_ERR("Fatal : Result is \"%s\" in %s at line %d", res.toString(), __FILE__, __LINE__); \
            std::abort(); \
        } \
    }
}  // namespace aph

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
}  // namespace aph::utils

namespace aph
{
#ifdef __cplusplus
    #ifndef MAKE_ENUM_FLAG
        #define MAKE_ENUM_FLAG(TYPE, ENUM_TYPE) \
            inline constexpr ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) \
            { \
                return ENUM_TYPE(((TYPE)a) | ((TYPE)b)); \
            } \
            inline ENUM_TYPE& operator|=(ENUM_TYPE& a, ENUM_TYPE b) \
            { \
                return (ENUM_TYPE&)(((TYPE&)a) |= ((TYPE)b)); \
            } \
            inline constexpr ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) \
            { \
                return ENUM_TYPE(((TYPE)a) & ((TYPE)b)); \
            } \
            inline ENUM_TYPE& operator&=(ENUM_TYPE& a, ENUM_TYPE b) \
            { \
                return (ENUM_TYPE&)(((TYPE&)a) &= ((TYPE)b)); \
            } \
            inline constexpr ENUM_TYPE operator~(ENUM_TYPE a) \
            { \
                return ENUM_TYPE(~((TYPE)a)); \
            } \
            inline constexpr ENUM_TYPE operator^(ENUM_TYPE a, ENUM_TYPE b) \
            { \
                return ENUM_TYPE(((TYPE)a) ^ ((TYPE)b)); \
            } \
            inline ENUM_TYPE& operator^=(ENUM_TYPE& a, ENUM_TYPE b) \
            { \
                return (ENUM_TYPE&)(((TYPE&)a) ^= ((TYPE)b)); \
            }
    #endif
#else
    #define MAKE_ENUM_FLAG(TYPE, ENUM_TYPE)
#endif

#define MAKE_ENUM_CLASS_FLAG(T) \
    inline T operator|(T a, T b) \
    { \
        return T(uint32_t(a) | uint32_t(b)); \
    } \
    inline T operator&(T a, T b) \
    { \
        return T(uint32_t(a) & uint32_t(b)); \
    } /* NOLINT(bugprone-macro-parentheses) */ \
    inline T operator~(T a) \
    { \
        return T(~uint32_t(a)); \
    } /* NOLINT(bugprone-macro-parentheses) */ \
    inline bool operator!(T a) \
    { \
        return uint32_t(a) == 0; \
    } \
    inline bool operator==(T a, uint32_t b) \
    { \
        return uint32_t(a) == b; \
    } \
    inline bool operator!=(T a, uint32_t b) \
    { \
        return uint32_t(a) != b; \
    }

}  // namespace aph

