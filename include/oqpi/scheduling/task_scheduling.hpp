#pragma once

#include "oqpi/scheduling/task.hpp"
#include "oqpi/scheduling/task_type.hpp"
#include "oqpi/scheduling/task_handle.hpp"
#include "oqpi/scheduling/parallel_group.hpp"
#include "oqpi/scheduling/sequence_group.hpp"

namespace oqpi {

    template<typename _Scheduler, typename _DefaultGroupContext, typename _DefaultTaskContext>
    struct helpers
    {
        //------------------------------------------------------------------------------------------
        using self_type = helpers<_Scheduler, _DefaultGroupContext, _DefaultTaskContext>;


        //------------------------------------------------------------------------------------------
        // Static instance
        static _Scheduler scheduler_;
        static _Scheduler& scheduler() { return scheduler_; }


        //------------------------------------------------------------------------------------------
        // Add a task to the scheduler
        inline static task_handle schedule_task(const task_handle &hTask)
        {
            return scheduler_.add(hTask);
        }
        //------------------------------------------------------------------------------------------
        inline static task_handle schedule_task(task_handle &&hTask)
        {
            return scheduler_.add(std::move(hTask));
        }

        //------------------------------------------------------------------------------------------
        // Creates AND adds a task to the scheduler, any callable object can be passed along.
        // All the following overloads end up calling this function.
        //
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _TaskContext, typename _Func, typename... _Args>
        inline static task_handle schedule_task(const std::string &name, task_priority prio, _Func &&f, _Args &&...args)
        {
            return self_type::schedule_task(make_task<_TaskType, _TaskContext>(name, prio, std::forward<_Func>(f), std::forward<_Args>(args)...));
        }

        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : user defined
        template<typename _TaskContext, typename _Func, typename... _Args>
        inline static task_handle schedule_task(const std::string &name, task_priority prio, _Func &&f, _Args &&...args)
        {
            return self_type::schedule_task<task_type::waitable, _TaskContext>(name, prio, std::forward<_Func>(f), std::forward<_Args>(args)...);
        }

        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : default
        template<typename _Func, typename... _Args>
        inline static task_handle schedule_task(const std::string &name, task_priority prio, _Func &&f, _Args &&...args)
        {
            return self_type::schedule_task<_DefaultTaskContext>(name, prio, std::forward<_Func>(f), std::forward<_Args>(args)...);
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : user defined
        template<typename _TaskContext, typename _Func, typename... _Args>
        inline static void fire_and_forget_task(const std::string &name, task_priority prio, _Func &&f, _Args &&...args)
        {
            self_type::schedule_task<task_type::fire_and_forget, _TaskContext>(name, prio, std::forward<_Func>(f), std::forward<_Args>(args)...);
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : default
        template<typename _Func, typename... _Args>
        inline static void fire_and_forget_task(const std::string &name, task_priority prio, _Func &&f, _Args &&...args)
        {
            self_type::fire_and_forget_task<_DefaultTaskContext>(name, prio, std::forward<_Func>(f), std::forward<_Args>(args)...);
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        // Create a task, the task is NOT added to the scheduler
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _TaskContext, typename _Func, typename... _Args>
        inline static auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            return oqpi::make_task<_TaskType, _TaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
        }
        //------------------------------------------------------------------------------------------
        // Type     : user defined
        // Context  : default
        template<task_type _TaskType, typename _Func, typename... _Args>
        inline static auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            return self_type::make_task<_TaskType, _DefaultTaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
        }


        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : user defined
        template<typename _TaskContext, typename _Func, typename... _Args>
        inline static auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            return self_type::make_task<task_type::waitable, _TaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
        }


        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : default
        template<typename _Func, typename... _Args>
        inline static auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            return self_type::make_task<task_type::waitable, _DefaultTaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : user defined
        template<typename _TaskContext, typename _Func, typename... _Args>
        inline static auto make_task_item(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            return self_type::make_task<task_type::fire_and_forget, _TaskContext, _Func>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : default
        template<typename _Func, typename... _Args>
        inline static auto make_task_item(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            return self_type::make_task<task_type::fire_and_forget, _DefaultTaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
        }


        //------------------------------------------------------------------------------------------
        // Create a parallel group, the group is NOT added to the scheduler
        //
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _GroupContext>
        inline static auto make_parallel_group(const std::string &name, task_priority prio = task_priority::normal, int32_t taskCount = 0, int32_t maxSimultaneousTasks = 0)
        {
            return oqpi::make_parallel_group<_TaskType, _GroupContext>(scheduler_, name, prio, taskCount, maxSimultaneousTasks);
        }

        //------------------------------------------------------------------------------------------
        // Type     : user defined
        // Context  : default
        template<task_type _TaskType>
        inline static auto make_parallel_group(const std::string &name, task_priority prio = task_priority::normal, int32_t taskCount = 0, int32_t maxSimultaneousTasks = 0)
        {
            return self_type::make_parallel_group<_TaskType, _DefaultGroupContext>(name, prio, taskCount, maxSimultaneousTasks);
        }


        //------------------------------------------------------------------------------------------
        // Creates a sequence of tasks, the group is NOT added to the scheduler
        //
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _GroupContext>
        inline static auto make_sequence_group(const std::string &name, task_priority prio = task_priority::normal)
        {
            return oqpi::make_sequence_group<_TaskType, _GroupContext>(scheduler_, name, prio);
        }

        //------------------------------------------------------------------------------------------
        // Type     : user defined
        // Context  : default
        template<task_type _TaskType>
        inline static auto make_sequence_group(const std::string &name, task_priority prio = task_priority::normal)
        {
            return self_type::make_sequence_group<_TaskType, _DefaultGroupContext>(name, prio);
        }


        //------------------------------------------------------------------------------------------
        // Group Context    : user defined
        // Task Context     : user defined
        template<typename _GroupContext, typename _TaskContext, typename _Func, typename _Partitioner>
        inline static void parallel_for(const std::string &name, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            oqpi::parallel_for<_GroupContext, _TaskContext>(scheduler_, name, partitioner, prio, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        template<typename _Func, typename _Partitioner>
        inline static void parallel_for(const std::string &name, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            self_type::parallel_for<_DefaultGroupContext, _DefaultTaskContext>(name, partitioner, prio, std::forward<_Func>(func));
        }
    };

    //----------------------------------------------------------------------------------------------
    template<typename _Scheduler, typename _DefaultGroupContext, typename _DefaultTaskContext>
    _Scheduler helpers<_Scheduler, _DefaultGroupContext, _DefaultTaskContext>::scheduler_;
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
