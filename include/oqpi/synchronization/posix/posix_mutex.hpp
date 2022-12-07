#pragma once

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"
#include "oqpi/synchronization/posix/posix_semaphore_wrapper.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Forward declaration of this platform mutex implementation
    using mutex_impl = class posix_mutex;

    //----------------------------------------------------------------------------------------------
    class posix_mutex
    {
    protected:
        //------------------------------------------------------------------------------------------
        using native_handle_type = sem_t*;

    protected:
        //------------------------------------------------------------------------------------------
        posix_mutex(const std::string &name, sync_object_creation_options creationOption, bool lockOnCreation)
            : sem_()
        {
            // A mutex is simply a binary semaphore!
            const auto initCount    = lockOnCreation ? 0u : 1u;
            sem_                    = posix_semaphore_wrapper(name, creationOption, initCount);
        }

        //------------------------------------------------------------------------------------------
        posix_mutex(posix_mutex &&other)
            : sem_(std::move(other.sem_))
        {
        }

        //------------------------------------------------------------------------------------------
        posix_mutex &operator=(posix_mutex &&rhs) 
        {
            if (this != &rhs)
            {
                sem_ = std::move(rhs.sem_);
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
        bool lock() 
        {
            return sem_.wait();
        }

        //------------------------------------------------------------------------------------------
        bool tryLock() 
        {
            return sem_.tryWait();
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool tryLockFor(const std::chrono::duration<_Rep, _Period> &relTime) 
        {
            return sem_.waitFor(relTime);
        }

        //------------------------------------------------------------------------------------------
        void unlock() 
        {
            const auto semValue = sem_.getValue();
            if (semValue >= 1)
            {
                oqpi_error("You cannot unlock a mutex more than once.");
                return;
            }

            sem_.post();
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        posix_mutex(const posix_mutex &) = delete;

        posix_mutex &operator=(const posix_mutex &) = delete;

    private:
        //------------------------------------------------------------------------------------------
        posix_semaphore_wrapper sem_;
    };
} /*oqpi*/
