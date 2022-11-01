#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>

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
        struct mutex_wrapper
        {
            ~mutex_wrapper()
            {
                pthread_mutex_destroy(&mutex);
            }

            pthread_mutex_t mutex;
        };

    protected:
        //------------------------------------------------------------------------------------------
        using native_handle_type = mutex_wrapper*;

    protected:
        //------------------------------------------------------------------------------------------
        posix_mutex(const std::string &name, sync_object_creation_options creationOption, bool lockOnCreation)
                : handle_(nullptr), name_("/" + name)
        {
            if (oqpi_failed(isNameValid()))
            {
                oqpi_error("The name \"%s\" you provided is not valid for shared memory.", name.c_str());
                return;
            }

            // Mode applies only to future accesses of the newly created file.
            auto mode           = S_IRUSR;
            auto flags          = O_CREAT | O_EXCL | O_RDWR;
            auto fileDescriptor = shm_open(name_.c_str(), flags, mode);

            // Open existing mutex.
            if (fileDescriptor == -1 && creationOption != sync_object_creation_options::create_if_nonexistent)
            {
                // If O_EXCL and O_CREAT are specified, and a shared memory object with the given name already exists,
                // an error is returned.

                // Remove flags (O_EXCL, O_CREAT) and try to open shared memory that already exists.
                flags &= ~O_CREAT;
                flags &= ~O_EXCL;

                while (fileDescriptor == -1)
                {
                    // Now since if the mode is still S_IRUSR, then shm_open with O_RDWR flags will fail until write permission given (after mutex was initialized).
                    fileDescriptor = shm_open(name_.c_str(), flags, 0);
                    if (fileDescriptor == -1)
                    {
                        if (errno == EACCES)
                        {
                            // If errno == EACCES, then caller does not have write permission on the object. Mutex is currently being initialized. Sleep 10 ms.
                            const auto timeToSleep = timespec{ 0, (long)1e7 };
                            nanosleep(&timeToSleep, NULL);
                        }
                        else
                        {
                            oqpi_error("shm_open failed with error code %d", errno);
                            oqpi_error("Was not able to properly get handle of global mutex.");
                            release();
                            return;
                        }
                    }
                }

                // Map the object into the caller's address space.
                handle_ = reinterpret_cast<mutex_wrapper *>(mmap(NULL, sizeof(*handle_), PROT_READ | PROT_WRITE,
                    MAP_SHARED, fileDescriptor, 0));
            }
            // Create new mutex.
            else if(fileDescriptor != -1 && creationOption != sync_object_creation_options::open_existing)
            {
                // Allocate shared memory.
                if(ftruncate(fileDescriptor, sizeof(mutex_wrapper)) == -1)
                {
                    oqpi_error("ftruncate() failed with error code %d", errno);
                    release();
                    close(fileDescriptor);
                    return;
                }

                // Map the object into the caller's address space.
                handle_ = reinterpret_cast<mutex_wrapper *>(mmap(NULL, sizeof(*handle_), PROT_READ | PROT_WRITE,
                    MAP_SHARED, fileDescriptor, 0));

                if(!initMutex(lockOnCreation))
                {
                    release();
                    close(fileDescriptor);
                    return;
                }

                // Now allow for read and write access.
                fchmod(fileDescriptor, S_IRUSR | S_IWUSR);
            }
            else
            {
                oqpi_error("Was not able to open or create a posix_mutex with the creation option specified.");
                release();
            }

            close(fileDescriptor);
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
            return pthread_mutex_lock(&handle_->mutex) == 0;
        }

        //------------------------------------------------------------------------------------------
        bool tryLock() 
        {
            return pthread_mutex_trylock(&handle_->mutex) == 0;
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool tryLockFor(const std::chrono::duration<_Rep, _Period> &relTime) 
        {
            auto nanoseconds    = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime);
            const auto secs     = std::chrono::duration_cast<std::chrono::seconds>(nanoseconds);
            nanoseconds         -= secs;
            const auto timeout  = timespec{secs.count(), nanoseconds.count()};

            return pthread_mutex_timedlock(&handle_->mutex, &timeout) == 0;
        }

        //------------------------------------------------------------------------------------------
        void unlock() 
        {
            const auto error = pthread_mutex_unlock(&handle_->mutex);
            if(error != 0)
            {
                oqpi_error("pthread_mutex_unlock failed with error number %d", error);
            }
        }

    private:
        //------------------------------------------------------------------------------------------
        void release()
        {
            if (handle_ != nullptr)
            {
                unmapSharedMemory();
                handle_ = nullptr;
            }

            if(isNameValid())
            {
                unlinkSharedMemory();
                name_ = "";
            }
        }

        //------------------------------------------------------------------------------------------
        void unmapSharedMemory()
        {
            // Removes any mappings for those entire pages containing any part of the address space of the process starting at handle_.
            const auto error = munmap(handle_, sizeof(*handle_));
            if (error == -1)
            {
                oqpi_error("munmap failed with error code %d", errno);
            }
        }

        //------------------------------------------------------------------------------------------
        void unlinkSharedMemory()
        {
            // Removes shared memory object name, and, once all processes have unmapped the object, de-allocates and destroys the contents
            // of the associated memory region.
            const auto error = shm_unlink(name_.c_str());
            if (error == -1)
            {
                oqpi_error("shm_unlink failed with error code %d", errno);
            }
        }

        //------------------------------------------------------------------------------------------
        bool initMutex(bool lockOnCreation)
        {
            auto attr  = pthread_mutexattr_t{};
            auto error = pthread_mutexattr_init(&attr);
            if (error != 0)
            {
                oqpi_error("pthread_mutexattr_init failed with error number %d", error);
                pthread_mutexattr_destroy(&attr);
                return false;
            }

            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

            error = pthread_mutex_init(&handle_->mutex, &attr);
            pthread_mutexattr_destroy(&attr);
            if (error != 0)
            {
                oqpi_error("pthread_mutex_init failed with error number %d", error);
                return false;
            }

            if (lockOnCreation)
            {
                oqpi_verify(lock());
            }

            return true;
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
