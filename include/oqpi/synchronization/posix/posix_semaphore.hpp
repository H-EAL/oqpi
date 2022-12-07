#pragma once

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"
#include "oqpi/synchronization/posix/posix_semaphore_wrapper.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Forward declaration of this platform semaphore implementation
    using semaphore_impl = class posix_semaphore;


    //----------------------------------------------------------------------------------------------
    class posix_semaphore
    {
    protected:
        //------------------------------------------------------------------------------------------
        using native_handle_type = sem_t*;

    protected:
        //------------------------------------------------------------------------------------------
        posix_semaphore(const std::string &name, sync_object_creation_options creationOption, int32_t initCount, int32_t maxCount)
            : maxCount_(maxCount)
            , sem_(name, creationOption, initCount)
        {
        }

        //------------------------------------------------------------------------------------------
        posix_semaphore(posix_semaphore &&other)
            : sem_(std::move(other.sem_))
            , maxCount_(other.maxCount_)
        {
        }

        //------------------------------------------------------------------------------------------
        posix_semaphore& operator =(posix_semaphore &&rhs)
        {
            if (this != &rhs)
            {
                sem_        = std::move(rhs.sem_);
                maxCount_   = rhs.maxCount_;
            }
            return (*this);
        }

    protected:
        //------------------------------------------------------------------------------------------
        // User interface
        native_handle_type getNativeHandle() const
        {
            return sem_.getHandle();
        }

        //------------------------------------------------------------------------------------------
        bool isValid() const
        {
            return sem_.isValid();
        }

        //------------------------------------------------------------------------------------------
        void notify(int32_t count)
        {
            for(auto i = 0; i < count; ++i)
            {
                sem_.post();
            }
        }

        //------------------------------------------------------------------------------------------
        void notifyAll()
        {
            notify(maxCount_);
        }

        //------------------------------------------------------------------------------------------
        bool tryWait()
        {
            return sem_.tryWait();
        }

        //------------------------------------------------------------------------------------------
        bool wait()
        {
            return sem_.wait();
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool waitFor(const std::chrono::duration<_Rep, _Period> &relTime)
        {
            sem_.waitFor(relTime);
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        posix_semaphore(const posix_semaphore &)                = delete;

        posix_semaphore& operator =(const posix_semaphore &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        int32_t                 maxCount_;
        posix_semaphore_wrapper sem_;
    };

} /*oqpi*/

