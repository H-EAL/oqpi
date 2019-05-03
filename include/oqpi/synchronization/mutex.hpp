#pragma once

#include "oqpi/platform.hpp"

// Interface
#include "oqpi/synchronization/interface/interface_mutex.hpp"
// Platform specific implementations
#if OQPI_PLATFORM_WIN
#	include "oqpi/synchronization/win/win_mutex.hpp"
#else
#	error No mutex implementation defined for the current platform
#endif

namespace oqpi {

    //----------------------------------------------------------------------------------------------
    template<template<typename> typename _Layer = local_sync_object>
    using mutex_interface = itfc::mutex<mutex_impl, _Layer>;
    //----------------------------------------------------------------------------------------------

#ifdef OQPI_USE_DEFAULT
    //----------------------------------------------------------------------------------------------
    using global_mutex = mutex_interface<global_sync_object>;
#endif

} /*oqpi*/
