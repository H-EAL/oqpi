#pragma once

#include <array>

#include "oqpi/platform.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    using sync_impl = class win_sync;

    //----------------------------------------------------------------------------------------------
    class win_sync
    {
    public:
        //------------------------------------------------------------------------------------------
        template<typename ..._SyncObjects>
        static auto wait_indefinitely_for_any(_SyncObjects &&...syncObjects)
        {
            auto handles = make_handle_array(std::forward<_SyncObjects>(syncObjects)...);
            return WaitForMultipleObjects(DWORD(handles.size()), handles.data(), FALSE, INFINITE);
        }

    private:
        //------------------------------------------------------------------------------------------
        template<typename _SyncObject>
        static void make_handle(const _SyncObject &syncObject, HANDLE &handle)
        {
            handle = syncObject.getNativeHandle();
        }

        //------------------------------------------------------------------------------------------
        template<typename ..._SyncObjects>
        static auto make_handle_array(_SyncObjects &&...syncObjects)
        {
            auto handles = std::array<HANDLE, sizeof...(syncObjects)>{};
            auto i = 0;
            (make_handle(syncObjects, handles[i++]), ...);
            return handles;
        }
    };

} /*oqpi*/
