#pragma once

#include "oqpi/platform.hpp"

// Thread interface
#include "oqpi/synchronization/interface/interface_semaphore.hpp"
// Platform specific implementations
#if OQPI_PLATFORM_WIN
#	include "oqpi/synchronization/win/win_semaphore.hpp"
#else
#	error No semaphore implementation defined for the current platform
#endif

namespace oqpi {

    using semaphore_interface = interface::semaphore<semaphore_impl, empty_layer>;

}