#pragma once

#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"


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
                : handle_(nullptr)
                , name_(name)
        {
            const auto isLocalSyncObject = name.empty();
            if (isLocalSyncObject && creationOption != sync_object_creation_options::open_existing)
            {
                // Create an unnamed semaphore.
                handle_ = new sem_t;

                const auto error = sem_init(handle_, 0, 0);
                if (error == -1)
                {
                    oqpi_error("sem_init failed with error %d", errno);
                    release();
                }
            }
            else
            {
                // Create a named semaphore.
                if (oqpi_failed(isNameValid()))
                {
                    oqpi_error("the name \"%s\" you provided is not valid for a posix semaphore.", name.c_str());
                    return;
                }

                // If both O_CREAT and O_EXCL are specified, then an error is returned if a semaphore with the given
                // name already exists. Otherwise it creates it.
                handle_ = sem_open(name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

                if(handle_ != SEM_FAILED && creationOption == sync_object_creation_options::open_existing)
                {
                    // We've created a semaphore even though given the open_existing creation option.
                    oqpi_error("Trying to open an existing event that doesn't exist.");
                    release();
                    return;
                }

                // Open a semaphore
                if(handle_ == SEM_FAILED &&
                   (creationOption == sync_object_creation_options::open_existing
                    || creationOption == sync_object_creation_options::open_or_create))
                {
                    handle_ = sem_open(name.c_str(), O_CREAT, S_IRUSR | S_IWUSR, 0);
                }

                if (handle_ == SEM_FAILED)
                {
                    oqpi_error("sem_open failed with error %d", errno);
                    release();
                }
            }
        }

        //------------------------------------------------------------------------------------------
        posix_event(posix_event &&rhs)
                : handle_(rhs.handle_)
                , name_(rhs.name_)
        {
            rhs.handle_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        ~posix_event()
        {
            release();
        }

        //------------------------------------------------------------------------------------------
        posix_event& operator =(posix_event &&rhs)
        {
            if (this != &rhs && !isValid())
            {
                handle_             = rhs.handle_;
                name_               = rhs.name_;
                rhs.handle_         = nullptr;
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
        void notify()
        {
            const auto error = sem_post(handle_);
            if(error == -1)
            {
                oqpi_error("sem_post failed with error code %d", errno);
            }
        }

        //------------------------------------------------------------------------------------------
        bool wait()
        {
            auto error = sem_wait(handle_);
            if(error == -1)
            {
                oqpi_error("sem_wait failed with error code %d", errno);
            }

            // Only if its a manual reset event, then we also want to signal other potential waiters.
            // To signal other potential waiters, increment lock. Snowball effect.
            if (_ResetPolicy::is_manual_reset_enabled())
            {
                error = sem_post(handle_);
                if (error == -1)
                {
                    oqpi_error("sem_post failed with error code %d", errno);
                }
            }

            return error != -1;
        }

        //------------------------------------------------------------------------------------------
        void reset()
        {
            _ResetPolicy::reset(handle_);
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool waitFor(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            auto nanoseconds  = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime);
            const auto secs   = std::chrono::duration_cast<std::chrono::seconds>(nanoseconds);
            nanoseconds       -= secs;

            timespec t;
            t.tv_sec    = secs.count();
            t.tv_nsec   = nanoseconds.count();

            auto error = sem_timedwait(handle_, &t);
            if(error == -1 && errno != ETIMEDOUT)
            {
                oqpi_error("sem_timedwait failed with error code %d", errno);
            }
            else if(error != -1)
            {
                // Wait was successful.
                // Only if its a manual reset event, then we also want to signal other potential waiters.
                // increment lock. Snowball effect.
                if (_ResetPolicy::is_manual_reset_enabled())
                {
                    error = sem_post(handle_);
                    if (error == -1)
                    {
                        oqpi_error("sem_post failed with error code %d", errno);
                    }
                }
            }
            return error != -1;
        }

    private:
        //------------------------------------------------------------------------------------------
        void release()
        {
            if (handle_)
            {
                const auto isLocalSyncObject = name_.empty();
                if(isLocalSyncObject)
                {
                    sem_destroy(handle_);
                    delete handle_;
                }
                else
                {
                    sem_close(handle_);
                    sem_unlink(name_.c_str());
                }
                handle_ = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        bool isNameValid() const
        {
            // Note that name must be in the form of /somename; that is, a null-terminated string of up to NAME_MAX
            // characters consisting of an initial slash, followed by one or more characters, none of which are slashes.
            return (name_.length() < NAME_MAX && name_.length() > 1 && name_[0] == '/'
                    && std::find(name_.begin() + 1, name_.end(), '/') == name_.end());
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        posix_event(const posix_event &)                = delete;
        posix_event& operator =(const posix_event &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        native_handle_type  handle_;
        std::string         name_;
    };


    //----------------------------------------------------------------------------------------------
    struct posix_event_manual_reset_policy
    {
        static bool is_manual_reset_enabled()
        {
            return true;
        }

        void reset(sem_t *handle)
        {
            auto semValue = 0;
            sem_getvalue(handle, &semValue);

            // Decrement internal counter to 0. Event is unsignaled.
            while(semValue != 0)
            {
                const auto error = sem_wait(handle);
                if(error == -1)
                {
                    oqpi_error("sem_wait failed with error code %d", errno);
                    oqpi_error("Was not able to reset the event.");
                    break;
                }
                sem_getvalue(handle, &semValue);
            }
        }
    };

} /*oqpi*/
