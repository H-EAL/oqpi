#pragma once

#include "oqpi/platform.hpp"

// Thread interface
#include "oqpi/threads/thread_interface.hpp"
// Platform specific implementations
#if OQPI_PLATFORM_WIN
#	include "oqpi/threads/win_thread.hpp"
#else
#	error No thread implementation defined for the current platform
#endif

namespace oqpi {

	using thread_interface = interface::thread<thread_impl, empty_layer>;

}