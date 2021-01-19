//
// Created by Npchitman on 2021/1/17.
//

#ifndef HAZELENGINE_CORE_H
#define HAZELENGINE_CORE_H

#ifdef HZ_PLATFORM_WINDOWS
    #ifdef HZ_BUILD_DLL
    #define HAZEL_API __declspec(dllexport)
    #else
    #define HAZEL_API __declspec(dllimport)
    #endif
#else
    #error  Hazel only supports Windows!
#endif

#define BIT(x) (1 << x)

#endif //HAZELENGINE_CORE_H
