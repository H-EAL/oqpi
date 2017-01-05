#pragma once

#include <vector>
#include <atomic>

#include "oqpi/scheduling/task_group.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Builds a sequence of tasks as such:
    //
    // [T0] -> [T1] -> [T2] -> ... -> [Tn-1] -> [Tn]
    //
    // [Tn] waits on [Tn-1] completion which waits on [Tn-2] completion etc...
    //
    // This group is not thread safe, meaning the user has to ensure thread safety herself when
    // adding tasks to this kind of group.
    //
    template<typename _Scheduler, task_type _TaskType, typename _GroupContext>
    class sequence_group final
        : public task_group<_Scheduler, _TaskType, _GroupContext>
    {
    public:
        //------------------------------------------------------------------------------------------
        sequence_group(_Scheduler &sc, std::string name, task_priority priority, int32_t nbTasks = 0)
            : task_group<_Scheduler, _TaskType, _GroupContext>(sc, std::move(name), priority)
            , currentIndex_(0)
        {
            tasks_.reserve(nbTasks);
        }

    public:
        //------------------------------------------------------------------------------------------
        virtual bool empty() const override final
        {
            return tasks_.empty();
        }

        //------------------------------------------------------------------------------------------
        // For debug purposes
        virtual void executeSingleThreadedImpl() override final
        {
            if (task_base::tryGrab())
            {
                for (auto &hTask : tasks_)
                {
                    if (oqpi_ensure(hTask.tryGrab()))
                    {
                        hTask.executeSingleThreaded();
                    }
                }
            }
        }

    protected:
        //------------------------------------------------------------------------------------------
        virtual void addTaskImpl(const task_handle &hTask) override final
        {
            tasks_.emplace_back(hTask);
        }

        //------------------------------------------------------------------------------------------
        // Executes the current task
        virtual void executeImpl() override final
        {
            oqpi_checkf(!empty(), "Invalid sequence");
            auto &currentTask = tasks_[currentIndex_];
            if (currentTask.tryGrab())
            {
                currentTask.execute();
            }
        }

        //------------------------------------------------------------------------------------------
        virtual void oneTaskDone() override final
        {
            if (++currentIndex_ < tasks_.size())
            {
                this->scheduler_.add(tasks_[currentIndex_]);
            }
            else
            {
                this->notifyGroupDone();
            }
        }

    private:
        // Tasks of the sequence
        std::vector<task_handle>    tasks_;
        // Index of the next task to execute
        int32_t                     currentIndex_;
    };
    //----------------------------------------------------------------------------------------------
    

    //----------------------------------------------------------------------------------------------
    template<typename _EventType, task_type _TaskType, typename _GroupContext, typename _Scheduler>
    inline auto make_sequence_group(_Scheduler &sc, const std::string &name, task_priority prio)
    {
        return make_task_group<sequence_group, _EventType, _TaskType, _GroupContext>(sc, name, prio);
    }
    //----------------------------------------------------------------------------------------------

} /*oqpi*/