#pragma once

#include "oqpi/scheduling/task_handle.hpp"
#include "oqpi/scheduling/task_notifier.hpp"
#include "oqpi/scheduling/task_group_base.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    template<typename _Dispatcher, task_type _TaskType, typename _GroupContext>
    class task_group
        : public task_group_base
        , public notifier<_TaskType>
        , public _GroupContext
    {
    public:
        //------------------------------------------------------------------------------------------
        task_group(_Dispatcher &disp, std::string name, task_priority priority)
            : task_group_base(std::move(name), priority)
            , notifier<_TaskType>(name_)
            , _GroupContext(this)
            , dispatcher_(disp)
        {}

        //------------------------------------------------------------------------------------------
        virtual ~task_group()
        {}

    public:
        //------------------------------------------------------------------------------------------
        // task_group_base implemented interface
        virtual void addTask(task_handle hTask) override final
        {
            if(hTask.isValid())
            {
                if (oqpi_ensuref(hTask.getParentGroup() == nullptr,
                    "This task (%s) is already bound to a group: %s", hTask.getName().c_str(), hTask.getParentGroup()->getName().c_str()))
                {
                    hTask.setParentGroup(shared_from_this());
                    addTaskImpl(hTask);
                    _GroupContext::group_onTaskAdded(hTask);
                }
            }
        }

        //------------------------------------------------------------------------------------------
        virtual void execute() override final
        {
            _GroupContext::group_onPreExecute();
            executeImpl();
        }

        //------------------------------------------------------------------------------------------
        virtual void executeSingleThreaded() override final
        {
            _GroupContext::group_onPreExecute();
            executeSingleThreadedImpl();
            _GroupContext::group_onPostExecute();

            task_base::done_.store(true);
            notifier<_TaskType>::notify();
        }

        //------------------------------------------------------------------------------------------
        virtual void wait() const override final
        {
            notifier<_TaskType>::wait();
        }

        //------------------------------------------------------------------------------------------
        virtual void activeWait() override
        {
            oqpi_checkf(false, "Not supported, fall back to wait");
            wait();
        }

    protected:
        //------------------------------------------------------------------------------------------
        // Implementation details interface
        virtual void addTaskImpl(const task_handle &hTask)  = 0;
        virtual void executeImpl()                          = 0;
        virtual void executeSingleThreadedImpl()            = 0;

    protected:
        //------------------------------------------------------------------------------------------
        // Called once all tasks of a group are done
        void notifyGroupDone()
        {
            task_base::setDone();
            notifier<_TaskType>::notify();
            _GroupContext::group_onPostExecute();
        }

    protected:
        alignas(_Dispatcher&)_Dispatcher &dispatcher_;
    };
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    template<template<typename, task_type, typename> class _TaskGroupType, task_type _TaskType, typename _GroupContext, typename _Dispatcher, typename... _Args>
    inline auto make_task_group(_Dispatcher &disp, const std::string &name, _Args &&...args)
    {
        return std::make_shared<_TaskGroupType<_Dispatcher, _TaskType, _GroupContext>>(disp, name, std::forward<_Args>(args)...);
    }
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
