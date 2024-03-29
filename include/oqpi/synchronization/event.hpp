#pragma once

#include "oqpi/platform.hpp"

// Interface
#include "oqpi/synchronization/interface/interface_event.hpp"
// Platform specific implementations
#if OQPI_PLATFORM_WIN
#	include "oqpi/synchronization/win/win_event.hpp"
#elif OQPI_PLATFORM_POSIX
#	include "oqpi/synchronization/posix/posix_event.hpp"
#else
#	error No event implementation defined for the current platform
#endif

namespace oqpi {

    //----------------------------------------------------------------------------------------------
    template<template<typename> typename _Layer = local_sync_object>
    using auto_reset_event_interface    = itfc::event<event_impl<event_auto_reset_policy_impl>,     _Layer>;

    //----------------------------------------------------------------------------------------------
    template<template<typename> typename _Layer = local_sync_object>
    using manual_reset_event_interface  = itfc::event<event_impl<event_manual_reset_policy_impl>,   _Layer>;


#ifdef OQPI_USE_DEFAULT
    //----------------------------------------------------------------------------------------------
    using auto_reset_event          = auto_reset_event_interface<>;
    using manual_reset_event        = manual_reset_event_interface<>;
    //----------------------------------------------------------------------------------------------
    using global_auto_reset_event   = auto_reset_event_interface<global_sync_object>;
    using global_manual_reset_event = manual_reset_event_interface<global_sync_object>;
#endif

} /*oqpi*/
