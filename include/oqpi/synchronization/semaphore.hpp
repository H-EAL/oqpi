#pragma once

#include "oqpi/platform.hpp"

// Interface
#include "oqpi/synchronization/interface/interface_semaphore.hpp"
// Platform specific implementations
#if OQPI_PLATFORM_WIN
#	include "oqpi/synchronization/win/win_semaphore.hpp"
#elif OQPI_PLATFORM_POSIX
#	include "oqpi/synchronization/posix/posix_semaphore.hpp"
#else
#	error No semaphore implementation defined for the current platform
#endif

namespace oqpi {

    //----------------------------------------------------------------------------------------------
    template<template<typename> typename _Layer = local_sync_object>
    using semaphore_interface = itfc::semaphore<semaphore_impl, _Layer>;
    //----------------------------------------------------------------------------------------------

#ifdef OQPI_USE_DEFAULT
    //----------------------------------------------------------------------------------------------
    using semaphore         = semaphore_interface<>;
    using global_semaphore  = semaphore_interface<global_sync_object>;
    //----------------------------------------------------------------------------------------------
#endif

} /*oqpi*/
