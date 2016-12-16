#pragma once

#include "oqpi/platform.hpp"

namespace oqpi {

	using thread_impl = class win_thread;

	class win_thread
	{
	protected:
        win_thread()
            : handle_(nullptr)
        {}

        ~win_thread()
        {
            if (handle_)
            {
                CloseHandle(handle_);
            }
        }

        using native_handle = HANDLE;
        using id            = DWORD;

        native_handle   getNativeHandle()   const { return handle_; }
        id              getId()             const { return id_;     }

        using thread_proc = DWORD(LPVOID);

        bool create(const thread_attributes &attributes, thread_proc &&threadProc)
        {
            const auto lpThreadAttributes   = nullptr;
            const auto dwStackSize          = DWORD{ attributes.stackSize };
            const auto lpStartAddress       = [](LPVOID lpParam) -> DWORD { return 0; };
            const auto lpParameter          = nullptr;
            const auto dwCreationFlags      = DWORD{ 0 }; // default creation flags
            const auto lpThreadId           = &id_;

            handle_ = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
            return handle_ != nullptr;
        }

    private:
        HANDLE handle_;
        DWORD  id_;
	};

    namespace this_thread {

        //----------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        inline void sleep_for(const std::chrono::duration<_Rep, _Period> &time)
        {
            const auto dwMilliseconds = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(time).count());
            Sleep(dwMilliseconds);
        }
        //----------------------------------------------------------------------------------------------

    } /*this_thread*/

} /*oqpi*/