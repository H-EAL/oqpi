#pragma once

#include "oqpi/platform.hpp"

namespace oqpi {

	using thread_impl = class win_thread;

	class win_thread
	{
    protected:
        //------------------------------------------------------------------------------------------
        win_thread()
            : handle_(nullptr)
        {}
        //------------------------------------------------------------------------------------------
        ~win_thread()
        {
            if (handle_)
            {
                CloseHandle(handle_);
            }
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        using native_handle = HANDLE;
        using id            = DWORD;
        //------------------------------------------------------------------------------------------
        native_handle   getNativeHandle()   const { return handle_; }
        id              getId()             const { return id_; }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        void join()
        {
            if (handle_)
            {
                WaitForSingleObject(handle_, INFINITE);
            }
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        using thread_proc = DWORD(WINAPI*)(LPVOID);
        //------------------------------------------------------------------------------------------
        template<typename _Target>
        thread_proc getThreadProc() const
        {
            return thread_proc{ [](LPVOID pData) -> DWORD
            {
                auto upTarget = std::unique_ptr<_Target>(static_cast<_Target*>(pData));
                if (upTarget != nullptr)
                {
                    (*upTarget)();
                }
                return 0;
            } };
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        bool create(const thread_attributes &attributes, thread_proc threadProc, void *pData)
        {
            // Specify if the handle can be inherited by child processes. NULL means it cannot.
            const auto lpThreadAttributes   = LPSECURITY_ATTRIBUTES{ nullptr };

            // Initial size of the thread's stack in bytes. 0 means use default
            const auto dwStackSize          = SIZE_T{ attributes.stackSize };

            // Thread function
            const auto lpStartAddress       = LPTHREAD_START_ROUTINE{ threadProc };

            // Data pointer
            const auto lpParameter          = LPVOID{ pData };

            // Creation flags
            const auto dwCreationFlags      = DWORD{ 0 };

            // This will receive the thread identifier
            const auto lpThreadId           = LPDWORD{ &id_ };


            handle_ = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
            if (handle_ == nullptr)
            {
                return false;
            }

            //setPriority(attributes.priority_);
            //setCoreAffinityMask(attributes.coreAffinityMask_);

            return true;
        }
        //------------------------------------------------------------------------------------------

        
        //------------------------------------------------------------------------------------------
        void setName(const char *name)
        {
            set_name(id_, name);
        }
    public:
        //------------------------------------------------------------------------------------------
        static void set_name(win_thread::id threadId, const char *name)
        {
            #pragma pack(push,8)
            struct THREADNAME_INFO
            {
                DWORD dwType;       // Must be 0x1000.
                LPCSTR szName;      // Pointer to name (in user addr space).
                DWORD dwThreadID;   // Thread ID (-1=caller thread).
                DWORD dwFlags;      // Reserved for future use, must be zero.
            };
            #pragma pack(pop)

            THREADNAME_INFO info;
            info.dwType = 0x1000;
            info.szName = name;
            info.dwThreadID = threadId;
            info.dwFlags = 0;

            __try
            {
                static const auto MS_VC_EXCEPTION = DWORD{ 0x406D1388 };
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        static id get_current_thread_id()
        {
            return GetCurrentThreadId();
        }
        //------------------------------------------------------------------------------------------

    private:
        HANDLE handle_;
        DWORD  id_;
	};

    namespace this_thread {

        //------------------------------------------------------------------------------------------
        inline void set_name(const char *name)
        {
            win_thread::set_name(GetCurrentThreadId(), name);
        }
        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        inline void sleep_for(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            const auto dwMilliseconds = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(relTime).count());
            SleepEx(dwMilliseconds, TRUE);
        }
        //------------------------------------------------------------------------------------------
        inline unsigned int get_current_core()
        {
            // Retrieves the number of the processor the current thread was running on during
            // the call to this function.
            return GetCurrentProcessorNumber();
        }
        //------------------------------------------------------------------------------------------
        inline void yield() noexcept
        {
            // Causes the calling thread to yield execution to another thread that is ready to run
            // on the current processor. The operating system selects the next thread to be executed.
            SwitchToThread();
        }
        //------------------------------------------------------------------------------------------
        inline void set_priority(thread_priority threadPriority)
        {
            //win_thread::set_priority(GetCurrentThread(), threadPriority);
        }
        //------------------------------------------------------------------------------------------
        inline void set_affinity_mask(core_affinity coreAffinity)
        {
            //win_thread::set_affinity_mask(GetCurrentThread(), coreAffinity);
        }
        //------------------------------------------------------------------------------------------

    } /*this_thread*/

} /*oqpi*/