#pragma once

// Define every supported platform to 0 so it can be used in #if statements
#define TARGET_PLATFORM_WIN		(0)
#define TARGET_PLATFORM_MAC		(0)
#define TARGET_PLATFORM_POSIX	(0)

// Define only the current platform to 1
#if defined(PLATFORM_WIN)
#	undef  TARGET_PLATFORM_WIN
#	define TARGET_PLATFORM_WIN		(1)
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   include <windows.h>
#elif defined(PLATFORM_MAC)
#	undef  TARGET_PLATFORM_MAC
#	define TARGET_PLATFORM_MAC		(1)
#elif defined(PLATFORM_POSIX)
#	undef  TARGET_PLATFORM_POSIX
#	define TARGET_PLATFORM_POSIX	(1)
#endif

#include <chrono>
#include <memory>