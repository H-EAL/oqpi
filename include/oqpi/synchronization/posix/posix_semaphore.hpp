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
                , name_(name.empty() ? "" : "/" + name)
        {
            const auto isLocalSyncObject = name_.empty();
            if(isLocalSyncObject && creationOption != sync_object_creation_options::open_existing)
            {
                // Create an unnamed semaphore.
                handle_ = new sem_t;

                const auto error = sem_init(handle_, 0, initCount);
                if(error == -1)
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
        }

        //------------------------------------------------------------------------------------------
        ~posix_semaphore()
        {
            release();
        }

        //------------------------------------------------------------------------------------------
        posix_semaphore(posix_semaphore &&other)
                : handle_(other.handle_)
                , name_(other.name_)
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
                name_               = rhs.name_;
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
            timespec t;
            if (clock_gettime(CLOCK_REALTIME, &t) == -1)
            {
                oqpi_error("clock_gettime failed with error code %d", errno);
                return false;
            }

            auto nanoseconds    = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime);
            const auto secs     = std::chrono::duration_cast<std::chrono::seconds>(nanoseconds);
            nanoseconds         -= secs;
            t.tv_sec            += secs.count();
            t.tv_nsec           += nanoseconds.count();

            const auto error    = sem_timedwait(handle_, &t);
            if(error == -1 && errno != ETIMEDOUT)
            {
                oqpi_error("sem_timedwait failed with error code %d", errno);
            }
            return error == 0;
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
        posix_semaphore(const posix_semaphore &)                = delete;
        posix_semaphore& operator =(const posix_semaphore &)    = delete;

    private:
        //------------------------------------------------------------------------------------------
        int         maxCount_;
        sem_t*      handle_;
        std::string name_;
    };

} /*oqpi*/

