//
// Created by npchitman on 6/28/21.
//

#ifndef APHRODITE_ENGINE_ASSERT_H
#define APHRODITE_ENGINE_ASSERT_H

#include <filesystem>

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Debug/Log.h"

#ifdef APH_ENABLE_ASSERTS

// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
#define APH_INTERNAL_ASSERT_IMPL(type, check, msg, ...) \
    {                                                  \
        if (!(check)) {                                \
            APH##type##ERROR(msg, __VA_ARGS__);         \
            APH_DEBUGBREAK();                           \
        }                                              \
    }

#define APH_INTERNAL_ASSERT_WITH_MSG(type, check, ...) APH_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define APH_INTERNAL_ASSERT_NO_MSG(type, check) APH_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", APH_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define APH_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define APH_INTERNAL_ASSERT_GET_MACRO(...) APH_EXPAND_MACRO(APH_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, APH_INTERNAL_ASSERT_WITH_MSG, APH_INTERNAL_ASSERT_NO_MSG))

// Currently accepts at least the condition and one additional parameter (the message) being optional
#define APH_ASSERT(...) APH_EXPAND_MACRO(APH_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__))
#define APH_CORE_ASSERT(...) APH_EXPAND_MACRO(APH_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__))
#else
#define APH_ASSERT(...)
#define APH_CORE_ASSERT(...)
#endif

#endif//APHRODITE_ENGINE_ASSERT_H
