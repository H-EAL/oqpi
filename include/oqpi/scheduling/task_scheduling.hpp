#pragma once

#include "oqpi/scheduling/task.hpp"
#include "oqpi/scheduling/task_type.hpp"
#include "oqpi/scheduling/task_handle.hpp"

namespace oqpi {

    template<typename _Dispatcher, typename _DefaultGroupContext, typename _DefaultTaskContext>
    struct td
    {
        //------------------------------------------------------------------------------------------
        static _Dispatcher dispatcher_;


        //------------------------------------------------------------------------------------------
        // Add a task to the scheduler
        inline static task_handle dispatch_task(const task_handle &hTask)
        {
            return dispatcher_.add(hTask);
        }
        //------------------------------------------------------------------------------------------
        inline static task_handle dispatch_task(task_handle &&hTask)
        {
            return dispatcher_.add(std::move(hTask));
        }

        //------------------------------------------------------------------------------------------
        // Creates AND adds a task to the scheduler, any callable object can be passed along.
        // All the following overloads end up calling this function.
        //
        // Type     : user defined
        // Context  : user defined
        template<task_type _TaskType, typename _TaskContext, typename _Func>
        inline static task_handle dispatch_task(const std::string &name, task_priority prio, _Func &&f)
        {
            return dispatch_task(make_task<_TaskType, _TaskContext>(name, prio, std::forward<_Func>(f)));
        }

        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : user defined
        template<typename _TaskContext, typename _Func>
        inline static task_handle dispatch_task(const std::string &name, task_priority prio, _Func &&f)
        {
            return dispatch_task<task_type::waitable, _TaskContext>(name, prio, std::forward<_Func>(f));
        }

        //------------------------------------------------------------------------------------------
        // Type     : waitable
        // Context  : default
        template<typename _Func>
        inline static task_handle dispatch_task(const std::string &name, task_priority prio, _Func &&f)
        {
            return dispatch_task<_DefaultTaskContext>(name, prio, std::forward<_Func>(f));
        }

        //------------------------------------------------------------------------------------------
        // Type     : fire_and_forget
        // Context  : user defined
        template<typename _TaskContext, typename _Func>
        inline static void fire_and_forget_task(const std::string &name, task_priority prio, _Func &&f)
        {
            dispatch_task<task_type::fire_and_forget, _TaskContext>(name, prio, std::forward<_Func>(f));
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
    };

    //----------------------------------------------------------------------------------------------
    template<typename _Dispatcher, typename _DefaultGroupContext, typename _DefaultTaskContext>
    _Dispatcher td<_Dispatcher, _DefaultGroupContext, _DefaultTaskContext>::dispatcher_;
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
