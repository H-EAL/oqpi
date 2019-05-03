#pragma once

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Forward declaration of this platform mutex implementation
    using mutex_impl = class win_mutex;


    //----------------------------------------------------------------------------------------------
    class win_mutex
    {
    protected:
        //------------------------------------------------------------------------------------------
        using native_handle_type = HANDLE;

    protected:
        //------------------------------------------------------------------------------------------
        win_mutex(const std::string &name, sync_object_creation_options creationOption)
            : handle_(nullptr)
        {
            if (creationOption == sync_object_creation_options::open_existing)
            {
                oqpi_check(!name.empty());
                handle_ = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, name.c_str());
            }
            else
            {
                handle_ = CreateMutexA(nullptr, TRUE, name.empty() ? nullptr : name.c_str());
                oqpi_check(creationOption == sync_object_creation_options::open_or_create || GetLastError() != ERROR_ALREADY_EXISTS);
            }

            oqpi_check(handle_ != nullptr);
        }

        //------------------------------------------------------------------------------------------
        ~win_mutex()
        {
            if (handle_)
            {
                CloseHandle(handle_);
                handle_ = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        win_mutex(win_mutex &&other)
            : handle_(other.handle_)
        {
            other.handle_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        win_mutex& operator =(win_mutex &&rhs)
        {
            if (this != &rhs)
            {
                handle_ = rhs.handle_;
                rhs.handle_ = nullptr;
            }
            return (*this);
        }

    protected:
        //------------------------------------------------------------------------------------------
        // User interface
        native_handle_type getNativeHandle() const
        {
            return handle_;
        }

        //------------------------------------------------------------------------------------------
        bool isValid() const
        {
            return handle_ != nullptr;
        }

        //------------------------------------------------------------------------------------------
        bool lock()
        {
            return internalWait(INFINITE, TRUE);
        }

        //------------------------------------------------------------------------------------------
        bool tryLock()
        {
            return internalWait(0, TRUE);
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool tryLockFor(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            const auto dwMilliseconds = DWORD(std::chrono::duration_cast<std::chrono::milliseconds>(relTime).count());
            return internalWait(dwMilliseconds, TRUE);
        }

        //------------------------------------------------------------------------------------------
        void unlock()
        {
            oqpi_verify(ReleaseMutex(handle_) != FALSE);
        }

    private:
        //------------------------------------------------------------------------------------------
        bool internalWait(DWORD dwMilliseconds, BOOL bAlertable)
        {
            const auto result = WaitForSingleObjectEx(handle_, dwMilliseconds, bAlertable);
            if (oqpi_failed(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT))
            {
                oqpi_error("WaitForSingleObjectEx failed with error code 0x%x", GetLastError());
            }
            return (result == WAIT_OBJECT_0);
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        win_mutex(const win_mutex &)                = delete;
        win_mutex& operator =(const win_mutex &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        HANDLE  handle_;
    };

} /*oqpi*/
