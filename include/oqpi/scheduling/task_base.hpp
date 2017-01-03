#pragma once

#include <atomic>
#include <memory>
#include <string>
#include "oqpi/scheduling/task_type.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    class task_group_base;
    //----------------------------------------------------------------------------------------------
    class task_base;
    //----------------------------------------------------------------------------------------------
    using task_group_sptr = std::shared_ptr<task_group_base>;
    //----------------------------------------------------------------------------------------------
    using task_uptr = std::unique_ptr<task_base>;
    using task_sptr = std::shared_ptr<task_base>;
    using task_wptr = std::weak_ptr<task_base>;
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Base class for all kind of tasks, unit tasks as well as groups
    class task_base
    {
    public:
        //------------------------------------------------------------------------------------------
        // Constructor
        task_base(std::string name, task_priority priority)
            : name_(std::move(name))
            , priority_(priority)
            , spParentGroup_(nullptr)
            , grabbed_(false)
            , done_(false)
        {}

        //------------------------------------------------------------------------------------------
        // Used as a base class for the whole task hierarchy
        virtual ~task_base() = default;

        //------------------------------------------------------------------------------------------
        // Can be moved
        task_base(task_base &&other)
            : name_(std::move(other.name_))
            , priority_(other.priority_)
            , spParentGroup_(std::move(other.spParentGroup_))
            , grabbed_(other.grabbed_.load())
            , done_(other.done_.load())
        {}

        //------------------------------------------------------------------------------------------
        task_base& operator =(task_base &&rhs)
        {
            if (this != &rhs)
            {
                name_           = std::move(rhs.name_);
                priority_       = rhs.priority_;
                spParentGroup_  = std::move(rhs.spParentGroup_);
                grabbed_        = rhs.grabbed_.load();
                done_           = rhs.done_.load();
            }
            return (*this);
        }

        //------------------------------------------------------------------------------------------
        // Copy disabled
        task_base(const task_base &)                = delete;
        task_base& operator =(const task_base &)    = delete;

    public:
        //------------------------------------------------------------------------------------------
        // Interface
        virtual void execute()                  = 0;
        virtual void executeSingleThreaded()    = 0;
        virtual void wait() const               = 0;
        virtual void activeWait()               = 0;

    public:
        //------------------------------------------------------------------------------------------
        // Accessors
        task_priority getPriority() const
        {
            return priority_;
        }

        void setParentGroup(const task_group_sptr &spParentGroup)
        {
            spParentGroup_ = spParentGroup;
        }

        const task_group_sptr& getParentGroup() const
        {
            return spParentGroup_;
        }

        bool tryGrab()
        {
            bool expected = false;
            return grabbed_.compare_exchange_strong(expected, true);
        }

        bool isGrabbed() const
        {
            return grabbed_.load();
        }

        bool isDone() const
        {
            return done_.load();
        }

        void setDone();

        const std::string& getName() const
        {
            return name_;
        }

    protected:
        //------------------------------------------------------------------------------------------
        // The name of the task used in various debug tools
        std::string         name_;
        // Relative priority of the task
        task_priority       priority_;
        // Optional parent group
        task_group_sptr     spParentGroup_;
        // Token that has to be acquired by anyone before executing the task
        std::atomic<bool>   grabbed_;
        // Flag flipped once the task execution is done
        std::atomic<bool>   done_;
    };

} /*oqpi*/
