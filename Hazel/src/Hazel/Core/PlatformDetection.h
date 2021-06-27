//
// Created by npchitman on 6/28/21.
//

#ifndef HAZEL_ENGINE_PLATFORMDETECTION_H
#define HAZEL_ENGINE_PLATFORMDETECTION_H

#ifdef __linux__
#define HZ_PLATFORM_LINUX
#elif defined(_WIN32)
#error "Windows is not support!"
#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
/* TARGET_OS_MAC exists on all the platforms
 * so we must check all of them (in this order)
 * to ensure that we're running on MAC
 * and not some other Apple platform */
#if TARGET_IPHONE_SIMULATOR == 1
#error "IOS simulator is not supported!"
#elif TARGET_OS_IPHONE == 1
#define HZ_PLATFORM_IOS
#error "IOS is not supported!"
#elif TARGET_OS_MAC == 1
#define HZ_PLATFORM_MACOS
#error "MacOS is not supported!"
#else
#error "Unknown Apple platform!"
#endif
#error "other operatiing system are not supported!"
/* We also have to check __ANDROID__ before __linux__
 * since android is based on the linux kernel
 * it has __linux__ defined */
#elif defined(__ANDROID__)
#define HZ_PLATFORM_ANDROID
#error "Android is not supported!"
#else
/* Unknown compiler/platform */
#error "Unknown platform!"
#endif

#endif//HAZEL_ENGINE_PLATFORMDETECTION_H
