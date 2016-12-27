#pragma once

#include "oqpi/platform.hpp"

// Thread interface
#include "oqpi/synchronization/interface/interface_event.hpp"
// Platform specific implementations
#if OQPI_PLATFORM_WIN
#	include "oqpi/synchronization/win/win_event.hpp"
#else
#	error No event implementation defined for the current platform
#endif

namespace oqpi {

    using auto_reset_event_interface    = interface::event<event_impl<event_auto_reset_policy_impl>    , empty_layer>;
    using manual_reset_event_interface  = interface::event<event_impl<event_manual_reset_policy_impl>  , empty_layer>;

}