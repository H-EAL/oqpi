#pragma once

#include <string>
#include <atomic>
#include <memory>

#include "oqpi/threads/thread_attributes.hpp"
#include "oqpi/scheduling/task_type.hpp"
#include "oqpi/scheduling/task_handle.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    class worker_base;
    //----------------------------------------------------------------------------------------------
    using worker_uptr = std::unique_ptr<worker_base>;
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Worker priorities, each worker can be assigned to one or several priorities.
    // This enum is a bitfield coupled with task priorities. 
    enum class worker_priority
    {
        wprio_high              = 1 << int(task_priority::high),
        wprio_above_normal      = 1 << int(task_priority::above_normal),
        wprio_normal            = 1 << int(task_priority::normal),
        wprio_below_normal      = 1 << int(task_priority::below_normal),
        wprio_low               = 1 << int(task_priority::low),

        wprio_any_normal        = wprio_above_normal    | wprio_normal      | wprio_below_normal,

        wprio_normal_or_low     = wprio_low             | wprio_any_normal,
        wprio_normal_or_high    = wprio_any_normal      | wprio_high,

        wprio_any               = wprio_high            | wprio_any_normal  | wprio_low,
    };
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // A config used to create one or several workers when registering to the dispatcher
    struct worker_config
    {
        thread_attributes   threadAttributes;
        worker_priority     workerPrio;
        int32_t             count;
    };
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Whether or not a worker priority is compatible with a task priority
    inline bool can_work_on_priority(worker_priority workerPriority, task_priority taskPriority)
    {
        return ((1 << int(taskPriority)) & int(workerPriority)) != 0;
    }
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Base class for workers, it's basically a wrapper around a thread with a notification
    // object to be able to wake it up/put it asleep.
    class worker_base
    {
    public:
        //------------------------------------------------------------------------------------------
        worker_base(int id, const worker_config &config)
            : id_(id)
            , config_(config)
        {}

        //------------------------------------------------------------------------------------------
        virtual ~worker_base()
        {}

    public:
        //------------------------------------------------------------------------------------------
        bool isAvailable() const
        {
            return !hTask_.isValid();
        }

        //------------------------------------------------------------------------------------------
        void assign(task_handle &&hTask)
        {
            oqpi_checkf(isAvailable(), "Trying to assign a new task (%s) to a busy worker: %s (%s)",
                hTask.getName().c_str(), config_.threadAttributes.name_.c_str(), hTask_.getName().c_str());

            hTask_ = std::move(hTask);
        }

        //------------------------------------------------------------------------------------------
        worker_priority getPriority() const
        {
            return config_.workerPrio;
        }

        //------------------------------------------------------------------------------------------
        bool canWorkOnPriority(task_priority taskPriority) const
        {
            return can_work_on_priority(getPriority(), taskPriority);
        }

        //------------------------------------------------------------------------------------------
        int getId() const
        {
            return id_;
        }

        //------------------------------------------------------------------------------------------
        const worker_config& getConfig() const
        {
            return config_;
        }

        //------------------------------------------------------------------------------------------
        std::string getName() const
        {
            return config_.threadAttributes.name_ + std::to_string(id_);
        }

    public:
        //------------------------------------------------------------------------------------------
        virtual void start()    = 0;
        //------------------------------------------------------------------------------------------
        virtual void stop()     = 0;
        //------------------------------------------------------------------------------------------
        virtual void join()     = 0;
        //------------------------------------------------------------------------------------------
        virtual void wait()     = 0;
        //------------------------------------------------------------------------------------------
        virtual bool tryWait()  = 0;
        //------------------------------------------------------------------------------------------
        virtual void notify()   = 0;

    protected:
        //------------------------------------------------------------------------------------------
        // Main function of the thread, see worker.h for the implementation
        virtual void run()      = 0;

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        worker_base(const worker_base &)             = delete;
        worker_base& operator =(const worker_base &) = delete;

    protected:
        // Id of this worker, useful when a config is shared between several workers
        const int           id_;
        // The config used to create this thread
        const worker_config config_;
        // The task the thread is currently working on, or an invalid handle if the worker is idle
        task_handle         hTask_;
    };
    //----------------------------------------------------------------------------------------------

} /*oqpi*/

