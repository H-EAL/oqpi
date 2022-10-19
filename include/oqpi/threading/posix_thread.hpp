#pragma once

#include <chrono>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <cstring>

#include "oqpi/platform.hpp"
#include "oqpi/error_handling.hpp"

// Only Real Time Scheduling policies (SCHED_FIFO or SCHED_RR) allow setting thread priority.
// SCHED_FIFO corresponds to first in first out, SCHED_RR to round-robin.
// Use SCHED_RR since Windows schedules threads in a round-robin fashion as well.
// Otherwise, check for modifying 'nice' value of non-RT policies.
#define POSIX_SCHED_POLICY SCHED_RR


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Type definition of this platform thread implementation, this is needed by interface::thread
    // to be able to select the right implementation.
    using thread_impl = class posix_thread;
    //----------------------------------------------------------------------------------------------
    class posix_thread
    {
    protected:
        //------------------------------------------------------------------------------------------
        static uint32_t hardware_concurrency()
        {
            static const auto logicalProcessorCount = uint32_t(sysconf(_SC_NPROCESSORS_ONLN));
            return logicalProcessorCount;
        }

    protected:
        //------------------------------------------------------------------------------------------
        // Default constructible
        posix_thread() noexcept
                : handle_(0)
        {}
        //------------------------------------------------------------------------------------------
        ~posix_thread()
        {
            // The interface should have taken care of either joining or terminating the thread
            if (oqpi_failed(handle_ == 0))
            {
                handle_ = 0;
            }
        }
        //------------------------------------------------------------------------------------------

    protected:
        //------------------------------------------------------------------------------------------
        // Movable
        posix_thread(posix_thread &&other) noexcept
                : handle_(other.handle_)
        {
            other.handle_   = 0;
        }
        //------------------------------------------------------------------------------------------
        posix_thread& operator =(posix_thread &&rhs)
        {
            if (this != &rhs && oqpi_ensure(handle_ == 0))
            {
                handle_     = rhs.handle_;
                rhs.handle_ = 0;
            }
            return (*this);
        }

    protected:
        //------------------------------------------------------------------------------------------
        template<typename _Launcher>
        bool create(const thread_attributes &attributes, void *pData)
        {
            // Thread function
            const auto lpStartAddress = [](void *pData) -> void*
            {
                auto upLauncher = std::unique_ptr<_Launcher>(static_cast<_Launcher *>(pData));
                if(oqpi_ensure(upLauncher != nullptr))
                {
                    (*upLauncher)();
                }
                return nullptr;
            };

            auto pThreadAttr    = (pthread_attr_t*)(nullptr);
            auto threadAttr     = pthread_attr_t{};
            auto error          = pthread_attr_init(&threadAttr);

            // On Linux init always succeeds (but portable and future-proof applications
            // should nevertheless handle a possible error return).
            if(error == 0)
            {
                pThreadAttr = &threadAttr;
                pthread_attr_setdetachstate(pThreadAttr, false);

                auto set = cpu_set_t{};
                // Clears set, so that it contains no CPUs.
                CPU_ZERO(&set);

                if(attributes.coreAffinityMask_ == core_affinity::all_cores)
                {
                    // Fill the mask with 1's. -1 is represented in binary as 11111111.
                    memset(&set, -1, sizeof(cpu_set_t));
                }
                else
                {
                    const auto coreNumber = static_cast<int32_t>(sqrt((double)(attributes.coreAffinityMask_)));
                    CPU_SET(coreNumber, &set);
                }

                pthread_attr_setaffinity_np(pThreadAttr, sizeof(cpu_set_t), &set);
                pthread_attr_setschedpolicy(pThreadAttr, POSIX_SCHED_POLICY);

                const auto posix_priority = sched_param{ posix_thread_priority(attributes.priority_) };
                pthread_attr_setschedparam(pThreadAttr, &posix_priority);

                pthread_attr_setstacksize(pThreadAttr, attributes.stackSize_);
            }
            else
            {
                oqpi_error("pthread_attr_init failed with error number: %d", error);
            }

            error = pthread_create(&handle_, pThreadAttr, lpStartAddress, pData);

            if(error != 0)
            {
                oqpi_error("pthread_create failed with error number: %d", error);
                return false;
            }

            if(pThreadAttr != nullptr)
            {
                pthread_attr_destroy(pThreadAttr);
            }

            return true;
        }

    protected:
        //------------------------------------------------------------------------------------------
        // Platform specific types
        using native_handle_type    = pthread_t;
        using id                    = pthread_t;

    protected:
        //------------------------------------------------------------------------------------------
        id                  getId()             const { return handle_;       }
        native_handle_type  getNativeHandle()   const { return handle_;       }
        bool                joinable()          const { return handle_ != 0;  }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        void join()
        {
            if (handle_ == 0)
            {
                oqpi_warning("Trying to join a non joinable thread.");
                return;
            }

            if (handle_ == pthread_self())
            {
                oqpi_warning("The current thread is the same as the thread attempted to join.");
                return;
            }

            const auto error = pthread_join(handle_, nullptr);
            if (error == 0)
            {
                handle_ = 0;
            }
            else
            {
                oqpi_error("pthread_join failed with error code: %i", error);
            }
        }
        //------------------------------------------------------------------------------------------
        void detach()
        {
            if (handle_ == 0)
            {
                oqpi_warning("Trying to detach a non joinable thread.");
                return;
            }

            const auto error = pthread_detach(handle_);
            if (error == 0)
            {
                handle_ = 0;
            }
            else
            {
                oqpi_error("pthread_detach failed with error code: %d", error);
            }
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        void cancelSynchronousIO()
        {
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        core_affinity getCoreAffinityMask() const
        {
            return get_core_affinity_mask(handle_);
        }
        //------------------------------------------------------------------------------------------
        void setCoreAffinityMask(core_affinity affinityMask)
        {
            set_core_affinity_mask(handle_, affinityMask);
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        thread_priority getPriority() const
        {
            return get_priority(handle_);
        }
        //------------------------------------------------------------------------------------------
        void setPriority(thread_priority priority)
        {
            set_priority(handle_, priority);
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        void setName(const char *name)
        {
            set_name(handle_, name);
        }
        //------------------------------------------------------------------------------------------


    public:
        //------------------------------------------------------------------------------------------
        static void set_core_affinity_mask(native_handle_type handle, core_affinity affinityMask)
        {
            auto set = cpu_set_t{};
            // Clears set, so that it contains no CPUs.
            CPU_ZERO(&set);

            if (affinityMask == core_affinity::all_cores)
            {
                // Fill the mask with 1's. -1 is represented in binary as 11111111.
                memset(&set, -1, sizeof(cpu_set_t));
            }
            else
            {
                const auto coreNumber = static_cast<int32_t>(sqrt((double)(affinityMask)));
                CPU_SET(coreNumber, &set);
            }

            // Make sure the selected mask is valid
            oqpi_check(static_cast<uint64_t>(affinityMask) < (1ull << hardware_concurrency()) || affinityMask == core_affinity::all_cores);

            const auto error = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &set);
            if(error != 0)
            {
                oqpi_error("pthread_setaffinity_np failed with error code %d", error);
            }
        }
        //------------------------------------------------------------------------------------------
        static core_affinity get_core_affinity_mask(native_handle_type handle)
        {
            auto affinity = core_affinity{};
            cpu_set_t set;
            pthread_getaffinity_np(handle, sizeof(core_affinity), &set);
            memcpy(&affinity, &set, sizeof(core_affinity));

            return affinity;
        }
        //------------------------------------------------------------------------------------------
        static void set_priority(native_handle_type handle, thread_priority priority)
        {
            const auto posixPriority = sched_param{ posix_thread_priority(priority) };
            pthread_setschedparam(handle, POSIX_SCHED_POLICY, &posixPriority);
        }
        //------------------------------------------------------------------------------------------
        static thread_priority get_priority(native_handle_type handle)
        {
            auto threadPriority = thread_priority::normal;
            auto posixPriority  = sched_param{};
            auto policy         = POSIX_SCHED_POLICY;
            const auto error    = pthread_getschedparam(handle, &policy, &posixPriority);

            if(error == 0)
            {
                const auto prioFrac = (posixPriority.sched_priority - get_min_priority()) / (float)(get_max_priority() - get_min_priority());
                threadPriority      = static_cast<thread_priority>(prioFrac * ((uint32_t) thread_priority::count - 1));
            }
            else
            {
                oqpi_warning("Unable to retrieve thread priority for thread %lu", get_current_thread_id());
            }

            return threadPriority;
        }
        //------------------------------------------------------------------------------------------
        static void set_name(native_handle_type handle, const char *name)
        {
            const auto error = pthread_setname_np(handle, name);
            if(error != 0)
            {
                // Name must be max 16 characters long.
                oqpi_warning("Unable to set name for thread, error code %d", error);
            }
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        static native_handle_type get_current_thread_id()
        {
            return pthread_self();
        }
        //------------------------------------------------------------------------------------------

    private:
        //------------------------------------------------------------------------------------------
        static inline int get_min_priority()
        {
            static const auto minPriority = sched_get_priority_min(POSIX_SCHED_POLICY);
            return minPriority;
        }
        //------------------------------------------------------------------------------------------
        static inline int get_max_priority()
        {
            static const auto maxPriority = sched_get_priority_max(POSIX_SCHED_POLICY);
            return maxPriority;
        }
        //------------------------------------------------------------------------------------------
        static int posix_thread_priority(thread_priority prio)
        {
            // Priority between 0 and 1.
            const auto prioFrac = static_cast<float>(prio) / (static_cast<float>(thread_priority::count) - 1);
            return get_min_priority() + prioFrac * (get_max_priority() - get_min_priority());
        }
        //------------------------------------------------------------------------------------------

    private:
        //------------------------------------------------------------------------------------------
        native_handle_type handle_;
    };


    namespace this_thread {

        //------------------------------------------------------------------------------------------
        inline void set_name(const char *name)
        {
            posix_thread::set_name(pthread_self(), name);
        }
        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        inline void sleep_for(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            pthread_sleep(std::chrono::duration_cast<std::chrono::milliseconds>(relTime).count());
        }
        //------------------------------------------------------------------------------------------
        inline uint32_t get_current_core()
        {
            // Retrieves the number of the processor the current thread was running on during
            // the call to this function.
            return sched_getcpu();
        }
        //------------------------------------------------------------------------------------------
        inline void yield() noexcept
        {
            // Causes the calling thread to yield execution to another thread that is ready to run
            // on the current processor. The operating system selects the next thread to be executed.
            pthread_yield();
        }
        //------------------------------------------------------------------------------------------
        inline void set_priority(thread_priority threadPriority)
        {
            posix_thread::set_priority(pthread_self(), threadPriority);
        }
        //------------------------------------------------------------------------------------------
        inline void set_affinity_mask(core_affinity coreAffinityMask)
        {
            posix_thread::set_core_affinity_mask(pthread_self(), coreAffinityMask);
        }
        //------------------------------------------------------------------------------------------
        inline auto get_id()
        {
            return posix_thread::get_current_thread_id();
        }
        //------------------------------------------------------------------------------------------

    } /*this_thread*/

} /*oqpi*/
