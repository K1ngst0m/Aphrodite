//
// Created by npchitman on 5/30/21.
//

#ifndef HAZEL_CORE_H
#define HAZEL_CORE_H

//#ifdef HZ_PLATFORM_LINUX
//#else
//#error Hazel only supports Linux

#ifdef HZ_ENABLE_ASSERTS
#define HZ_ASSERT(x, ...)                                                      \
  {                                                                            \
    if (!(x)) {                                                                \
      HZ_ERROR("Assertion Failed: {0}", __VA_ARGS__);                          \
      __debugbreak();                                                          \
    }                                                                          \
  }
#define HZ_CORE_ASSERT(x, ...)                                                 \
  {                                                                            \
    if (!(x)) {                                                                \
      HZ_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__);                     \
      __debugbreak();                                                          \
    }                                                                          \
  }
#else
#define HZ_ASSERT(x, ...)
#define HZ_CORE_ASSERT(x, ...)
#endif
#define BIT(x) (1 << x)

#define HZ_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#endif // HAZEL_CORE_H
