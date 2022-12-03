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
                , unlockHandle_(nullptr)
                , name_("/" + name)
        {
            // Named posix semaphores have name requirements.
            if (oqpi_failed(isNameValid()))
            {
                oqpi_error("The name \"%s\" you provided is not valid for a posix semaphore.", name.c_str());
                return;
            }

            // Need to make sure these two mutexes (handle_ and unlockHandle_) are initialized atomically. 
            // Otherwise not thread safe. An example of a bad race condition that could arise:
            // Thread 1 creates handle_ with lockOnCreation = true. Yields.
            // Thread 2 opens handle_ with lockOnCreation = false. Then creates unlockHandle_ and locks it.
            // Thread 1 opens unlockHandle_.
            // Now handle_ and unlockHandle_ are locked, which will stop it from handle_ from being unlocked (check unlock()).
            executeAtomically([this, creationOption, lockOnCreation]()
            {
                // A mutex is simply a binary semaphore!
                if (!initBinarySemaphore(handle_, creationOption, name_, lockOnCreation))
                {
                    oqpi_error("Failed to create mutex %s with creation options specified.", name_);
                    release();
                    return;
                }

                // Create the helper mutex which will be used by the unlock() function.
                // When the main mutex is locked (is unlockable) the helper mutex is unlocked (is lockable).
                const auto helperLockOnCreation = !lockOnCreation;
                if (!initBinarySemaphore(unlockHandle_, sync_object_creation_options::open_or_create, getNameOfUnlockHandle(), helperLockOnCreation))
                {
                    oqpi_error("Failed to create the helper mutex (unlockHandle_) for mutex %s.", name_);
                    release();
                }
            });
        }

        //------------------------------------------------------------------------------------------
        ~posix_mutex() 
        {
            release();
        }

        //------------------------------------------------------------------------------------------
        posix_mutex(posix_mutex &&other)
                : handle_(other.handle_), unlockHandle_(other.unlockHandle_), name_(other.name_)
        {
            other.handle_       = nullptr;
            other.unlockHandle_ = nullptr;
            other.name_         = "";
        }

        //------------------------------------------------------------------------------------------
        posix_mutex &operator=(posix_mutex &&rhs) 
        {
            if (this != &rhs && !isValid())
            {
                handle_             = rhs.handle_;
                unlockHandle_       = rhs.unlockHandle_;
                name_               = rhs.name_;
                rhs.handle_         = nullptr;
                rhs.unlockHandle_   = nullptr;
                rhs.name_           = "";
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
            return lock(handle_);
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
            oqpi_ensure(lock(unlockHandle_));

            // The unlockHandle_ is used to make unlocking handle_ thread safe.
            // 
            // Without it there could be this situation:
            // Two threads call unlock() at the same time.
            // Thread 1 gets semValue of 0 from sem_getvalue, then yields.
            // Thread 2 gets semValue of 0 from sem_getvalue and proceeds to unlock, semValue = 1.
            // Thread 1 still has a semValue of 0 and unlocks the mutex, semValue = 2.
            // Thread 1 and 2 have unlocked the mutex.
            // semValue = 2, and the mutex can now be locked by two threads.
            auto semValue = 0;
            sem_getvalue(handle_, &semValue);
            if (semValue >= 1)
            {
                oqpi_error("You cannot unlock a mutex more than once.");
                return;
            }

            unlock(handle_);

            unlock(unlockHandle_);
        }

    private:
        //------------------------------------------------------------------------------------------
        bool initBinarySemaphore(native_handle_type &handle, sync_object_creation_options creationOption, const std::string &name, bool lockOnCreation)
        {
            // If both O_CREAT and O_EXCL are specified, then it succeeds if it creates a new semaphore.
            // An error is returned if a semaphore with the given name already exists.
            const auto initCount = lockOnCreation ? 0u : 1u;
            handle = sem_open(name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, initCount);

            if(handle != SEM_FAILED && creationOption == sync_object_creation_options::open_existing)
            {
                // We've created a semaphore even though given the open_existing creation option.
                oqpi_error("Trying to open an existing semaphore that doesn't exist.");
                return false;
            }

            // Open a semaphore
            if(handle == SEM_FAILED &&
               (creationOption == sync_object_creation_options::open_existing
                || creationOption == sync_object_creation_options::open_or_create))
            {
                handle = sem_open(name.c_str(), O_CREAT, S_IRUSR | S_IWUSR, 0);
            }

            if (handle == SEM_FAILED)
            {
                oqpi_error("sem_open failed with error %d", errno);
                return false;
            }

            return true;
        }

        //------------------------------------------------------------------------------------------
        template<typename _Func>
        void executeAtomically(_Func &&func)
        {
            native_handle_type tempHandle;
            const auto tempHandleName = name_ + "_a";

            oqpi_ensure(initBinarySemaphore(tempHandle, sync_object_creation_options::open_or_create, tempHandleName, false));
            oqpi_ensure(lock(tempHandle));

            func();

            unlock(tempHandle);
            release(tempHandle, tempHandleName);
        }

        //------------------------------------------------------------------------------------------
        void release()
        {
            release(handle_, name_);
            release(unlockHandle_, getNameOfUnlockHandle());
        }

        //------------------------------------------------------------------------------------------
        void release(native_handle_type &handle, const std::string &name)
        {
            if (handle)
            {
                sem_close(handle);
                sem_unlink(name.c_str());
                handle = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        bool isNameValid() const
        {
            const auto nameToCheck = getNameOfUnlockHandle();
            // Note that name must be in the form of /somename; that is, a null-terminated string of up to NAME_MAX
            // characters consisting of an initial slash, followed by one or more characters, none of which are slashes.
            return (nameToCheck.length() < NAME_MAX && nameToCheck.length() > 1 && nameToCheck[0] == '/'
                    && std::find(nameToCheck.begin() + 1, nameToCheck.end(), '/') == nameToCheck.end());
        }

        //------------------------------------------------------------------------------------------
        std::string getNameOfUnlockHandle() const
        {
            return name_ + "_lock";
        }

        //------------------------------------------------------------------------------------------
        bool lock(native_handle_type handle)
        {
            const auto error = sem_wait(handle);
            if (error == -1)
            {
                oqpi_error("sem_wait failed with error code %d", errno);
            }
            return error == 0;
        }

        //------------------------------------------------------------------------------------------
        void unlock(native_handle_type handle)
        {
            auto error = sem_post(handle);
            if (error == -1)
            {
                oqpi_error("sem_post failed with error code %d", errno);
            }
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
        //------------------------------------------------------------------------------------------
        // This is used to make the posix_mutex::unlock() thread-safe.
        // Has opposite semaphore value (1 or 0) to handle_. unlockHandle_ == !handle_.
        native_handle_type  unlockHandle_;
    };
} /*oqpi*/
