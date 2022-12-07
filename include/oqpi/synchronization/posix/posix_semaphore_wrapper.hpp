#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    class posix_semaphore_wrapper
    {
    public:
        //----------------------------------------------------------------------------------------------
        posix_semaphore_wrapper()
            : handle_(nullptr)
            , name_()
        {}

        //------------------------------------------------------------------------------------------
        posix_semaphore_wrapper(const std::string &name, sync_object_creation_options creationOption, int32_t initCount)
            : handle_(nullptr)
            , name_(name.empty() ? "" : "/" + name)
        {
            const auto isLocalSyncObject = name_.empty();
            if (isLocalSyncObject && creationOption != sync_object_creation_options::open_existing)
            {
                if (!initLocalSemaphore(creationOption, initCount))
                {
                    release();
                }
            }
            else
            {
                if (!initGlobalSemaphore(creationOption, initCount))
                {
                    release();
                }
            }
        }

        //------------------------------------------------------------------------------------------
        ~posix_semaphore_wrapper()
        {
            release();
        }

    public:
        //------------------------------------------------------------------------------------------
        posix_semaphore_wrapper(posix_semaphore_wrapper &&other)
            : handle_(other.handle_)
            , name_(other.name_)
        {
            other.handle_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        posix_semaphore_wrapper &operator =(posix_semaphore_wrapper &&rhs)
        {
            if (this != &rhs && !isValid())
            {
                handle_     = rhs.handle_;
                name_       = rhs.name_;
                rhs.handle_ = nullptr;
            }
            return (*this);
        }

    public:
        //------------------------------------------------------------------------------------------
        // User interface
        sem_t *getHandle() const
        {
            return handle_;
        }

        //------------------------------------------------------------------------------------------
        bool isValid() const
        {
            return handle_ != nullptr;
        }

    public:
        //------------------------------------------------------------------------------------------
        void post()
        {
            auto error = sem_post(handle_);
            if (error == -1)
            {
                oqpi_error("sem_post failed with error code %d", errno);
            }
        }

        //------------------------------------------------------------------------------------------
        bool tryWait()
        {
            const auto error = sem_trywait(handle_);
            // errno is set to EAGAIN if the decrement cannot be immediately performed (i.e. semaphore has value 0).
            if (error == -1 && errno != EAGAIN)
            {
                oqpi_error("sem_trywait failed with error code %d", errno);
            }
            return error == 0;
        }

        //------------------------------------------------------------------------------------------
        bool wait()
        {
            const auto error = sem_wait(handle_);
            if (error == -1)
            {
                oqpi_error("sem_wait failed with error code %d", errno);
            }
            return error == 0;
        }
        
        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        bool waitFor(const std::chrono::duration<_Rep, _Period> &relTime)
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

        //------------------------------------------------------------------------------------------
        int32_t getValue()
        {
            auto semValue = 0;
            sem_getvalue(handle_, &semValue);
            return semValue;
        }

    private:
        //------------------------------------------------------------------------------------------
        bool initLocalSemaphore(sync_object_creation_options creationOption, uint32_t initCount)
        {
            // Create an unnamed semaphore.
            handle_ = new sem_t;
        
            // If pshared has the value 0, then the semaphore is shared between the threads of a process.
            constexpr auto pshared  = 0;
            const auto error        = sem_init(handle_, pshared, initCount);
            if (error == -1)
            {
                oqpi_error("sem_init failed with error %d", errno);
                return false;
            }

            return true;
        }

        //------------------------------------------------------------------------------------------
        bool initGlobalSemaphore(sync_object_creation_options creationOption, uint32_t initCount)
        {
            // Create a named semaphore.
            if (oqpi_failed(isNameValid()))
            {
                oqpi_error("The name \"%s\" you provided is not valid for a posix semaphore.", name_.c_str());
                return false;
            }

            // If both O_CREAT and O_EXCL are specified, then an error is returned if a semaphore with the given
            // name already exists. Otherwise it creates it.
            handle_ = sem_open(name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, initCount);

            if (handle_ != SEM_FAILED && creationOption == sync_object_creation_options::open_existing)
            {
                // We've created a semaphore even though given the open_existing creation option.
                oqpi_error("Trying to open an existing semaphore that doesn't exist.");
                return false;
            }

            // Open a semaphore
            if (handle_ == SEM_FAILED &&
                (creationOption == sync_object_creation_options::open_existing
                    || creationOption == sync_object_creation_options::open_or_create))
            {
                handle_ = sem_open(name.c_str(), O_CREAT, S_IRUSR | S_IWUSR, 0);
            }

            if (handle_ == SEM_FAILED)
            {
                oqpi_error("sem_open failed with error %d", errno);
                return false;
            }

            return true;
        }

        //------------------------------------------------------------------------------------------
        bool isNameValid()
        {
            // Note that name must be in the form of /somename; that is, a null-terminated string of up to NAME_MAX
            // characters consisting of an initial slash, followed by one or more characters, none of which are slashes.
            return (name_.length() < NAME_MAX && name_.length() > 1 && name_[0] == '/'
                && std::find(name_.begin() + 1, name_.end(), '/') == name_.end());
        }

    private:
        //------------------------------------------------------------------------------------------
        void release()
        {
            if (handle_)
            {
                const auto isLocalSyncObject = name_.empty();
                if (isLocalSyncObject)
                {
                    releaseLocalSemaphore();
                }
                else
                {
                    releaseGlobalSemaphore();
                }
                handle_ = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        void releaseLocalSemaphore()
        {
            sem_destroy(handle_);
            delete handle_;
        }

        //------------------------------------------------------------------------------------------
        void releaseGlobalSemaphore()
        {
            sem_close(handle_);
            sem_unlink(name.c_str());
        }


    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        posix_semaphore_wrapper(const posix_semaphore_wrapper &) = delete;

        posix_semaphore_wrapper &operator=(const posix_semaphore_wrapper &) = delete;

    private:
        //------------------------------------------------------------------------------------------
        sem_t       *handle_;
        //------------------------------------------------------------------------------------------
        std::string name_;
    };

} /*oqpi*/
