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
        template<task_type _TaskType, typename _TaskContext, typename _Func>
        inline static task_handle schedule_task(const std::string &name, task_priority prio, _Func &&f)
        {
            return schedule_task(make_task<_TaskType, _TaskContext>(name, prio, std::forward<_Func>(f)));
        }

        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : user defined
        template<typename _TaskContext, typename _Func>
        inline static task_handle schedule_task(const std::string &name, task_priority prio, _Func &&f)
        {
            return schedule_task<task_type::waitable, _TaskContext>(name, prio, std::forward<_Func>(f));
        }

        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : default
        template<typename _Func>
        inline static task_handle schedule_task(const std::string &name, task_priority prio, _Func &&f)
        {
            return schedule_task<_DefaultTaskContext>(name, prio, std::forward<_Func>(f));
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : user defined
        template<typename _TaskContext, typename _Func>
        inline static void fire_and_forget_task(const std::string &name, task_priority prio, _Func &&f)
        {
            schedule_task<task_type::fire_and_forget, _TaskContext>(name, prio, std::forward<_Func>(f));
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : default
        template<typename _Func>
        inline static void fire_and_forget_task(const std::string &name, task_priority prio, _Func &&f)
        {
            fire_and_forget_task<_DefaultTaskContext>(name, prio, std::forward<_Func>(f));
        }
        //------------------------------------------------------------------------------------------


        //------------------------------------------------------------------------------------------
        // Create a task, the task is NOT added to the scheduler
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _TaskContext, typename _Func, typename... _Args>
        inline static auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
        {
            using tuple_type = std::tuple<std::decay_t<_Func>, std::decay_t<_Args>...>;
            using task_type = task<_TaskType, _TaskContext, tuple_type>;
            return std::make_shared<task_type>
            (
                name,
                priority,
                tuple_type(std::forward<_Func>(func), std::forward<_Args>(args)...)
            );
        }
        //------------------------------------------------------------------------------------------
        // Type     : user defined
        // Context  : user defined
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
            return make_task<task_type::fire_and_forget, _TaskContext, _Func>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
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
        inline static auto make_parallel_group(const std::string &name, task_priority prio, int32_t taskCount = 0, int32_t maxSimultaneousTasks = 0)
        {
            return oqpi::make_parallel_group<_TaskType, _GroupContext>(scheduler_, name, prio, taskCount, maxSimultaneousTasks);
        }

        //------------------------------------------------------------------------------------------
        // Type     : user defined
        // Context  : default
        template<task_type _TaskType>
        inline static auto make_parallel_group(const std::string &name, task_priority prio, int32_t taskCount = 0, int32_t maxSimultaneousTasks = 0)
        {
            return self_type::make_parallel_group<_TaskType, _DefaultGroupContext>(name, prio, taskCount, maxSimultaneousTasks);
        }


        //------------------------------------------------------------------------------------------
        // Creates a sequence of tasks, the group is NOT added to the scheduler
        //
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _GroupContext>
        inline static auto make_sequence_group(const std::string &name, task_priority prio, int32_t taskCount = 0)
        {
            return make_sequence_group<_TaskType, _GroupContext>(scheduler_, name, prio, taskCount);
        }

        //------------------------------------------------------------------------------------------
        // Type     : user defined
        // Context  : default
        template<task_type _TaskType>
        inline static auto make_sequence_group(const std::string &name, task_priority prio, int32_t taskCount = 0)
        {
            return self_type::make_sequence_group<_TaskType, _DefaultGroupContext>(name, prio, taskCount);
        }
    };

    //----------------------------------------------------------------------------------------------
    template<typename _Scheduler, typename _DefaultGroupContext, typename _DefaultTaskContext>
    _Scheduler helpers<_Scheduler, _DefaultGroupContext, _DefaultTaskContext>::scheduler_;
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
