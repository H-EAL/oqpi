#pragma once

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Forward declaration of this platform semaphore implementation
    using semaphore_impl = class win_semaphore;


    //----------------------------------------------------------------------------------------------
    class win_semaphore
    {
    protected:
        //------------------------------------------------------------------------------------------
        using native_handle_type = HANDLE;

    protected:
        //------------------------------------------------------------------------------------------
        win_semaphore(const std::string &name, sync_object_creation_options creationOption, int32_t initCount, int32_t maxCount)
            : initCount_(initCount)
            , maxCount_(maxCount)
            , handle_(nullptr)
        {
            if (creationOption == sync_object_creation_options::open_existing)
            {
                oqpi_check(!name.empty());
                handle_ = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name.c_str());
            }
            else
            {
                handle_ = CreateSemaphoreA(nullptr, initCount_, maxCount_, name.empty() ? nullptr : name.c_str());
                if (creationOption == sync_object_creation_options::create_if_nonexistent && GetLastError() == ERROR_ALREADY_EXISTS)
                {
                    CloseHandle(handle_);
                    handle_ = nullptr;
                }
            }
        }

        //------------------------------------------------------------------------------------------
        ~win_semaphore()
        {
            if (handle_)
            {
                CloseHandle(handle_);
                handle_ = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        win_semaphore(win_semaphore &&other)
            : handle_(other.handle_)
            , initCount_(other.initCount_)
            , maxCount_(other.maxCount_)
        {
            other.handle_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        win_semaphore& operator =(win_semaphore &&rhs)
        {
            if (this != &rhs)
            {
                handle_     = rhs.handle_;
                initCount_  = rhs.initCount_;
                maxCount_   = rhs.maxCount_;
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
        void notify(int32_t count)
        {
            auto previousCount = LONG{ 0 };
            oqpi_verify(ReleaseSemaphore(handle_, LONG{ count }, &previousCount) != 0);
        }

        //------------------------------------------------------------------------------------------
        void notifyAll()
        {
            notify(maxCount_);
        }

        //------------------------------------------------------------------------------------------
        bool tryWait()
        {
            return internalWait(0, TRUE);
        }

        //------------------------------------------------------------------------------------------
        bool wait()
        {
            return internalWait(INFINITE, TRUE);
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool waitFor(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            const auto dwMilliseconds = DWORD(std::chrono::duration_cast<std::chrono::milliseconds>(relTime).count());
            return internalWait(dwMilliseconds, TRUE);
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
        win_semaphore(const win_semaphore &)                = delete;
        win_semaphore& operator =(const win_semaphore &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        LONG    initCount_;
        LONG    maxCount_;
        HANDLE  handle_;
    };

} /*oqpi*/
