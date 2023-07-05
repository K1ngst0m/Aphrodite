#ifndef VKLCOMMON_H_
#define VKLCOMMON_H_

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stack>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <set>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "logger.h"

#define BACKWARD_HAS_DW 1
#define BACKWARD_HAS_BACKTRACE_SYMBOL 1
#include <backward-cpp/backward.hpp>

namespace {
using namespace backward;
class TracedException : public std::runtime_error
{
public:
    TracedException() : std::runtime_error(_get_trace()) {}

private:
    std::string _get_trace()
    {
        std::ostringstream ss;

        StackTrace    stackTrace;
        TraceResolver resolver;
        stackTrace.load_here();
        resolver.load_stacktrace(stackTrace);

        for(std::size_t i = 0; i < stackTrace.size(); ++i)
        {
            const ResolvedTrace trace = resolver.resolve(stackTrace[i]);

            ss << "#" << i << " at " << trace.object_function << "\n";
        }

        return ss.str();
    }
};

backward::SignalHandling sh;
}

namespace aph
{
enum class Result
{
    SUCCESS       = 0,
    NOT_READY     = 1,
    TIMEOUT       = 2,
    INCOMPLETE    = 5,
    ERROR_UNKNOWN = -1,
};

enum class BaseType
{
    BOOL   = 0,
    CHAR   = 1,
    INT    = 2,
    UINT   = 3,
    UINT64 = 4,
    HALF   = 5,
    FLOAT  = 6,
    DOUBLE = 7,
    STRUCT = 8,
};

struct ImageInfo
{
    uint32_t             width  = {};
    uint32_t             height = {};
    std::vector<uint8_t> data   = {};
};
}  // namespace aph

namespace aph::utils
{
#ifdef __GNUC__
#    define leading_zeroes(x) ((x) == 0 ? 32 : __builtin_clz(x))
#    define trailing_zeroes(x) ((x) == 0 ? 32 : __builtin_ctz(x))
#    define trailing_ones(x) __builtin_ctz(~uint32_t(x))
#    define leading_zeroes64(x) ((x) == 0 ? 64 : __builtin_clzll(x))
#    define trailing_zeroes64(x) ((x) == 0 ? 64 : __builtin_ctzll(x))
#    define trailing_ones64(x) __builtin_ctzll(~uint64_t(x))
#elif defined(_MSC_VER)
namespace Internal
{
static inline uint32_t clz(uint32_t x)
{
    unsigned long result;
    if(_BitScanReverse(&result, x))
        return 31 - result;
    else
        return 32;
}

static inline uint32_t ctz(uint32_t x)
{
    unsigned long result;
    if(_BitScanForward(&result, x))
        return result;
    else
        return 32;
}

static inline uint32_t clz64(uint64_t x)
{
    unsigned long result;
    if(_BitScanReverse64(&result, x))
        return 63 - result;
    else
        return 64;
}

static inline uint32_t ctz64(uint64_t x)
{
    unsigned long result;
    if(_BitScanForward64(&result, x))
        return result;
    else
        return 64;
}
}  // namespace Internal

#    define leading_zeroes(x) ::Util::Internal::clz(x)
#    define trailing_zeroes(x) ::Util::Internal::ctz(x)
#    define trailing_ones(x) ::Util::Internal::ctz(~uint32_t(x))
#    define leading_zeroes64(x) ::Util::Internal::clz64(x)
#    define trailing_zeroes64(x) ::Util::Internal::ctz64(x)
#    define trailing_ones64(x) ::Util::Internal::ctz64(~uint64_t(x))
#else
#    error "Implement me."
#endif
}  // namespace aph::utils

namespace aph::utils
{
constexpr uint32_t calculateFullMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
template <typename T>
typename std::underlying_type<T>::type getUnderLyingType(T value)
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}
std::shared_ptr<ImageInfo>                loadImageFromFile(std::string_view path, bool isFlipY = false);
std::array<std::shared_ptr<ImageInfo>, 6> loadSkyboxFromFile(std::array<std::string_view, 6> paths);
std::string                               readFile(const std::string& filename);

template <typename T>
inline void forEachBit64(uint64_t value, const T& func)
{
    while(value)
    {
        uint32_t bit = trailing_zeroes64(value);
        func(bit);
        value &= ~(1ull << bit);
    }
}

template <typename T>
inline void forEachBit(uint32_t value, const T& func)
{
    while(value)
    {
        uint32_t bit = trailing_zeroes(value);
        func(bit);
        value &= ~(1u << bit);
    }
}

}  // namespace aph::utils

namespace aph
{
#ifdef APH_DEBUG
#    define APH_ASSERT(x) \
        do \
        { \
            if(!bool(x)) \
            { \
                CM_LOG_ERR("Error at %s:%d.", __FILE__, __LINE__); \
                LOG_FLUSH(); \
                throw TracedException(); \
            } \
        } while(0)
#else
#    define APH_ASSERT(x) ((void)0)
#endif
}  // namespace aph

#endif  // VKLCOMMON_H_
