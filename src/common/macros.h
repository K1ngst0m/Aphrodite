#pragma once

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