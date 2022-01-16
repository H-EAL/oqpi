#pragma once

#include "oqpi/platform.hpp"

//--------------------------------------------------------------------------------------------------
// Platform specific implementations of synchronization functions
#if OQPI_PLATFORM_WIN
#	include "oqpi/synchronization/win/win_sync.hpp"
#elif OQPI_PLATFORM_POSIX
#	include "oqpi/synchronization/posix/posix_sync.hpp"
#else
#	error No sync functions implementation defined for the current platform
#endif

namespace oqpi {

    //----------------------------------------------------------------------------------------------
    using sync = sync_impl;

} /*oqpi*/
