#pragma once

#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"


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
                : handle_(nullptr)
                , name_("/" + name)
        {
            // Create a named semaphore.
            if (oqpi_failed(isNameValid()))
            {
                oqpi_error("the name \"%s\" you provided is not valid for a posix semaphore.", name.c_str());
                return;
            }

            // A mutex is simply a binary semaphore!

            // If both O_CREAT and O_EXCL are specified, then an error is returned if a semaphore with the given
            // name already exists. Otherwise it creates it.
            const auto initCount = lockOnCreation ? 0u : 1u;
            handle_ = sem_open(name_.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, initCount);

            if(handle_ != SEM_FAILED && creationOption == sync_object_creation_options::open_existing)
            {
                // We've created a semaphore even though given the open_existing creation option.
                oqpi_error("Trying to open an existing semaphore that doesn't exist.");
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

        //------------------------------------------------------------------------------------------
        ~posix_mutex() 
        {
            release();
        }

        //------------------------------------------------------------------------------------------
        posix_mutex(posix_mutex &&other)
                : handle_(other.handle_), name_(other.name_) 
        {
            other.handle_   = nullptr;
            other.name_     = "";
        }

        //------------------------------------------------------------------------------------------
        posix_mutex &operator=(posix_mutex &&rhs) 
        {
            if (this != &rhs && !isValid())
            {
                handle_     = rhs.handle_;
                name_       = rhs.name_;
                rhs.handle_ = nullptr;
                rhs.name_   = "";
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
            const auto error = sem_wait(handle_);
            if(error == -1)
            {
                oqpi_error("sem_wait failed with error code %d", errno);
            }
            return error == 0;
        }

        //------------------------------------------------------------------------------------------
        bool tryLock() 
        {
            const auto error = sem_trywait(handle_);
            // errno is set to EAGAIN if the decrement cannot be immediately performed (i.e. semaphore has value 0).
            if(error == -1 && errno != EAGAIN)
            {
                oqpi_error("sem_trywait failed with error code %d", errno);
            }
            return error == 0;
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool tryLockFor(const std::chrono::duration<_Rep, _Period> &relTime) 
        {
            auto nanoseconds  = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime);
            const auto secs   = std::chrono::duration_cast<std::chrono::seconds>(nanoseconds);
            nanoseconds       -= secs;

            timespec t;
            t.tv_sec    = secs.count();
            t.tv_nsec   = nanoseconds.count();

            const auto error = sem_timedwait(handle_, &t);
            if(error == -1 && errno != ETIMEDOUT)
            {
                oqpi_error("sem_timedwait failed with error code %d", errno);
            }
            return error == 0;
        }

        //------------------------------------------------------------------------------------------
        void unlock() 
        {
            auto semValue = 0;
            sem_getvalue(handle_, &semValue);
            if (semValue > 1)
            {
                oqpi_error("You cannot unlock a mutex more than once.");
                return;
            }

            auto error = sem_post(handle_);
            if(error == -1)
            {
                oqpi_error("sem_post failed with error code %d", errno);
            }
        }

    private:
        //------------------------------------------------------------------------------------------
        void release()
        {
            if (handle_)
            {
                sem_close(handle_);
                sem_unlink(name_.c_str());
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
        posix_mutex(const posix_mutex &) = delete;

        posix_mutex &operator=(const posix_mutex &) = delete;

    private:
        //------------------------------------------------------------------------------------------
        native_handle_type  handle_;
        std::string         name_;
    };
} /*oqpi*/
