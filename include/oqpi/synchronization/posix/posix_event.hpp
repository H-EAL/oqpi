#pragma once

#include <limits.h>

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"
#include "oqpi/synchronization/posix/posix_semaphore_wrapper.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Forward declaration of this platform semaphore implementation
    using event_manual_reset_policy_impl = struct posix_event_manual_reset_policy;
    //----------------------------------------------------------------------------------------------
    template<typename _ResetPolicy> class posix_event;
    //----------------------------------------------------------------------------------------------
    template<typename _ResetPolicy>
    using event_impl = class posix_event<_ResetPolicy>;
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    template<typename _ResetPolicy>
    class posix_event
            : protected _ResetPolicy
    {
    protected:
        //------------------------------------------------------------------------------------------
        using native_handle_type = sem_t*;

    protected:
        //------------------------------------------------------------------------------------------
        posix_event(const std::string &name, sync_object_creation_options creationOption)
            : sem_(name, creationOption, 0u)
        {
        }

        //------------------------------------------------------------------------------------------
        posix_event(posix_event &&rhs)
            : sem_(std::move(rhs.sem_))
        {
        }

        //------------------------------------------------------------------------------------------
        posix_event& operator =(posix_event &&rhs)
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
        void notify()
        {
            sem_.post();
        }

        //------------------------------------------------------------------------------------------
        bool wait()
        {
            if (!sem_.wait())
            {
                return false;
            }

            // Only if its a manual reset event, then we also want to signal other potential waiters.
            // To signal other potential waiters, increment lock. Snowball effect.
            if (_ResetPolicy::is_manual_reset_enabled())
            {
                sem_.post();
            }

            return true;
        }

        //------------------------------------------------------------------------------------------
        void reset()
        {
            _ResetPolicy::reset(sem_);
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool waitFor(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            if (!sem_.waitFor(relTime))
            {
                return false;
            }

            // Only if its a manual reset event, then we also want to signal other potential waiters.
            // increment lock. Snowball effect.
            if (_ResetPolicy::is_manual_reset_enabled())
            {
                sem_.post();
            }

            return true;
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        posix_event(const posix_event &)                = delete;
        posix_event& operator =(const posix_event &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        posix_semaphore_wrapper sem_;
    };


    //----------------------------------------------------------------------------------------------
    struct posix_event_manual_reset_policy
    {
        //------------------------------------------------------------------------------------------
        static bool is_manual_reset_enabled()
        {
            return true;
        }

        //------------------------------------------------------------------------------------------
        void reset(posix_semaphore_wrapper &sem)
        {
            auto semValue = sem.getValue();

            // Decrement internal counter to 0. Event is unsignaled.
            while (semValue != 0)
            {
                if (!sem.wait())
                {
                    oqpi_error("Was not able to reset event.");
                    break;
                }

                semValue = sem.getValue();
            }
        }
    };

} /*oqpi*/
