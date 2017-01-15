#pragma once

#include "oqpi/synchronization/event.hpp"
#include "oqpi/scheduling/task.hpp"
#include "oqpi/scheduling/task_type.hpp"
#include "oqpi/scheduling/task_handle.hpp"
#include "oqpi/scheduling/parallel_group.hpp"
#include "oqpi/scheduling/sequence_group.hpp"
#include "oqpi/parallel_algorithms/parallel_for.hpp"


namespace oqpi {

    template
    <
          typename _Scheduler
        // The context to use for making groups
        , typename _DefaultGroupContext = empty_group_context
        // The context to use for making tasks
        , typename _DefaultTaskContext  = empty_task_context
        // Type of events used for notification when a task is done
        , typename _EventType           = manual_reset_event_interface<>
    >
    struct helpers
    {
        //------------------------------------------------------------------------------------------
        using self_type = helpers<_Scheduler, _DefaultGroupContext, _DefaultTaskContext, _EventType>;


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
            auto spTask = self_type::make_task<_TaskType, _TaskContext>(name, prio, std::forward<_Func>(f), std::forward<_Args>(args)...);
            return self_type::schedule_task(std::move(spTask));
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
            return oqpi::make_task<_TaskType, _EventType, _TaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
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
        template<task_type _TaskType, typename _GroupContext, typename _TaskContext, typename _Func, typename _Partitioner>
        inline static auto make_parallel_for_task_group(const std::string &name, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            return oqpi::make_parallel_for_task_group<_TaskType, _EventType, _GroupContext, _TaskContext>(scheduler_, name, partitioner, prio, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        template<task_type _TaskType, typename _Func, typename _Partitioner>
        inline static auto make_parallel_for_task_group(const std::string &name, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            return self_type::make_parallel_for_task_group<_TaskType, _DefaultGroupContext, _DefaultTaskContext>(name, partitioner, prio, std::forward<_Func>(func));
        }


        //------------------------------------------------------------------------------------------
        // Group Context    : user defined
        // Task Context     : user defined
        // Partitioner      : user defined
        // Priority         : user defined
        template<typename _GroupContext, typename _TaskContext, typename _Func, typename _Partitioner>
        inline static void parallel_for(const std::string &name, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            oqpi::parallel_for<_EventType, _GroupContext, _TaskContext>(scheduler_, name, partitioner, prio, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : user defined
        // Task Context     : user defined
        // Partitioner      : simple_partitioner
        // Priority         : normal
        template<typename _GroupContext, typename _TaskContext, typename _Func>
        inline static void parallel_for(const std::string &name, int32_t firstIndex, int32_t lastIndex, _Func &&func)
        {
            const auto priority     = task_priority::normal;
            const auto partitioner  = oqpi::simple_partitioner(firstIndex, lastIndex, scheduler_.workersCount(priority));
            self_type::parallel_for<_GroupContext, _TaskContext>(name, partitioner, priority, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : user defined
        // Task Context     : user defined
        // Partitioner      : simple_partitioner
        // Priority         : normal
        template<typename _GroupContext, typename _TaskContext, typename _Func>
        inline static void parallel_for(const std::string &name, int32_t elementCount, _Func &&func)
        {
            self_type::parallel_for<_GroupContext, _TaskContext>(name, 0, elementCount, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        // Partitioner      : user defined
        // Priority         : user defined
        template<typename _Func, typename _Partitioner>
        inline static void parallel_for(const std::string &name, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            self_type::parallel_for<_DefaultGroupContext, _DefaultTaskContext>(name, partitioner, prio, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        // Partitioner      : simple_partitioner
        // Priority         : normal
        template<typename _Func>
        inline static void parallel_for(const std::string &name, int32_t firstIndex, int32_t lastIndex, _Func &&func)
        {
            self_type::parallel_for<_DefaultGroupContext, _DefaultTaskContext>(name, firstIndex, lastIndex, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        // Partitioner      : simple_partitioner
        // Priority         : normal
        template<typename _Func>
        inline static void parallel_for(const std::string &name, int32_t elementCount, _Func &&func)
        {
            self_type::parallel_for(name, 0, elementCount, std::forward<_Func>(func));
        }


        //------------------------------------------------------------------------------------------
        // Group Context    : user defined
        // Task Context     : user defined
        // Partitioner      : user defined
        // Priority         : user defined
        template<typename _GroupContext, typename _TaskContext, typename _Func, typename _Container, typename _Partitioner>
        inline static void parallel_for_each(const std::string &name, _Container &container, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            self_type::parallel_for<_GroupContext, _TaskContext>(name, partitioner, prio,
                [&container, func = std::forward<_Func>(func)](int32_t elementIndex)
            {
                func(container[elementIndex]);
            });
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        // Partitioner      : user defined
        // Priority         : user defined
        template<typename _Func, typename _Container, typename _Partitioner>
        inline static void parallel_for_each(const std::string &name, _Container &container, const _Partitioner &partitioner, task_priority prio, _Func &&func)
        {
            self_type::parallel_for_each<_DefaultGroupContext, _DefaultTaskContext>(name, container, partitioner, prio, std::forward<_Func>(func));
        }

        //------------------------------------------------------------------------------------------
        // Group Context    : default
        // Task Context     : default
        // Partitioner      : simple_partitioner
        // Priority         : normal
        template<typename _Func, typename _Container>
        inline static void parallel_for_each(const std::string &name, _Container &container, _Func &&func)
        {
            self_type::parallel_for(name, 0, int32_t(container.size()),
                [&container, func = std::forward<_Func>(func)](int32_t elementIndex)
            {
                func(container[elementIndex]);
            });
        }
    };

    //----------------------------------------------------------------------------------------------
    template<typename _Scheduler, typename _DefaultGroupContext, typename _DefaultTaskContext, typename _EventType>
    _Scheduler helpers<_Scheduler, _DefaultGroupContext, _DefaultTaskContext, _EventType>::scheduler_;
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
