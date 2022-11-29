#pragma once

#include "oqpi/platform.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    using sync_impl = class posix_sync;

    //----------------------------------------------------------------------------------------------
    class posix_sync
    {
    public:
        //------------------------------------------------------------------------------------------
        template<typename ..._SyncObjects, bool _False = false>
        static auto wait_indefinitely_for_any(_SyncObjects &&...syncObjects)
        {
            static_assert(_False, "Not implemented yet.");
            return 0;
        }
    };

} /*oqpi*/
