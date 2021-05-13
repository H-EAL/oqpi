#pragma once

#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"
#include "oqpi/synchronization/sync_common.hpp"


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
                , handle_(nullptr)
                , isLocalSyncObject_(name.empty())
        {
            if(isLocalSyncObject_ && creationOption != sync_object_creation_options::open_existing)
            {
                // Create an unnamed semaphore.
                handle_ = new sem_t;

                const auto error = sem_init(handle_, 0, 0);
                if(error == -1)
                {
                    oqpi_error("sem_init failed with error %d", errno);
                }
            }
            else
            {
                // Create a named semaphore.
                if (oqpi_failed(isNameValid(name)))
                {
                    oqpi_error("the name \"%s\" you provided is not valid for a posix semaphore.", name.c_str());
                    return;
                }

                // Create a new semaphore.
                if (creationOption == sync_object_creation_options::create_if_nonexistent)
                {
                    // An error is returned if a semaphore with the given name already exists.
                    handle_ = sem_open(name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, initCount);

                }
                // Create or open a semaphore.
                else
                {
                    // If a semaphore with name doesn't exist, then it is created.
                    handle_ = sem_open(name.c_str(), O_CREAT, S_IRUSR | S_IWUSR, initCount);
                }

                if (handle_ == SEM_FAILED)
                {
                    oqpi_error("sem_open failed with error %d", errno);
                }
            }
        }

        //------------------------------------------------------------------------------------------
        ~posix_semaphore()
        {
            if (handle_)
            {
                if(isLocalSyncObject_)
                {
                    sem_destroy(handle_);
                    delete handle_;
                }
                else
                {
                    sem_close(handle_);
                }
                handle_ = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        posix_semaphore(posix_semaphore &&other)
                : handle_(other.handle_)
                , isLocalSyncObject_(other.isLocalSyncObject_)
                , maxCount_(other.maxCount_)
        {
            other.handle_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        posix_semaphore& operator =(posix_semaphore &&rhs)
        {
            if (this != &rhs && !isValid())
            {
                handle_             = rhs.handle_;
                isLocalSyncObject_  = rhs.isLocalSyncObject_;
                maxCount_           = rhs.maxCount_;
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
        void notify(int32_t count)
        {
            for(auto i = 0; i < count; ++i)
            {
                auto error = sem_post(handle_);
                if(error == -1)
                {
                    oqpi_error("sem_post failed with error code %d", errno);
                }
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
            const auto error = sem_trywait(handle_);
            // errno is set to EAGAIN if the decrement cannot be immediately performed (i.e. semaphore has value 0).
            if(error == -1 && errno != EAGAIN)
            {
                oqpi_error("sem_trywait failed with error code %d", errno);
            }
            return error == 0;
        }

        //------------------------------------------------------------------------------------------
        bool wait()
        {
            const auto error = sem_wait(handle_);
            if(error == -1)
            {
                oqpi_error("sem_wait failed with error code %d", errno);
            }
            return error == 0;
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

            const auto error = sem_timedwait(handle_, &t);
            if(error == -1 && errno != ETIMEDOUT)
            {
                oqpi_error("sem_timedwait failed with error code %d", errno);
            }
            return error == 0;
        }

    private:
        //------------------------------------------------------------------------------------------
        bool isNameValid(const std::string &name) const
        {
            // Note that name must be in the form of /somename; that is, a null-terminated string of up to NAME_MAX
            // characters consisting of an initial slash, followed by one or more characters, none of which are slashes.
            return (name.length() < NAME_MAX && name.length() > 1 && name[0] == '/'
                    && std::find(name.begin() + 1, name.end(), '/') == name.end());
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        posix_semaphore(const posix_semaphore &)                = delete;
        posix_semaphore& operator =(const posix_semaphore &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        int     maxCount_;
        sem_t*  handle_;
        bool    isLocalSyncObject_;
    };

} /*oqpi*/

