#pragma once

// Define every supported platform to 0 so it can be used in #if statements
#define OQPI_PLATFORM_WIN	(0)
#define OQPI_PLATFORM_MAC	(0)
#define OQPI_PLATFORM_POSIX	(0)

// Define only the current platform to 1
#if defined(PLATFORM_WIN)
#	undef  OQPI_PLATFORM_WIN
#	define OQPI_PLATFORM_WIN	(1)
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   include <windows.h>
#elif defined(PLATFORM_MAC)
#	undef  OQPI_PLATFORM_MAC
#	define OQPI_PLATFORM_MAC	(1)
#elif defined(PLATFORM_POSIX)
#	undef  OQPI_PLATFORM_POSIX
#	define OQPI_PLATFORM_POSIX	(1)
#endif
