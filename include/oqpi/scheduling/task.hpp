#pragma once

#include <memory>
#include "oqpi/scheduling/task_base.hpp"
#include "oqpi/scheduling/task_notifier.hpp"


namespace oqpi {

    template<task_type _TaskType, typename _TaskContext, typename _Func>
    class task final
        : public task_base
        , public notifier<_TaskType>
        , public _TaskContext
    {
        //------------------------------------------------------------------------------------------
        using self_type     = task<_TaskType, _TaskContext, _Func>;
        using notifier_type = notifier<_TaskType>;

    public:
        //------------------------------------------------------------------------------------------
        task(std::string name, _Func func, task_priority priority)
            : task_base(std::move(name), priority)
            , notifier_type(name_)
            , _TaskContext(this)
            , func_(std::move(func))
        {}

        //------------------------------------------------------------------------------------------
        // Movable
        task(self_type &&other)
            : task_base(std::move(other))
            , notifier_type(std::move(other))
            , _TaskContext(std::move(other))
        {}

        //------------------------------------------------------------------------------------------
        self_type& operator =(self_type &&rhs)
        {
            if (this != &rhs)
            {
                task_base::operator =(std::move(rhs));
                notifier_type::operator =(std::move(rhs));
                _TaskContext::operator =(std::move(rhs));
                func_ = std::move(rhs.func_);
            }
            return (*this);
        }
        
        //------------------------------------------------------------------------------------------
        // Not copyable
        task(const self_type &)                     = delete;
        self_type& operator =(const self_type &)    = delete;

    public:
        //------------------------------------------------------------------------------------------
        virtual void execute() override final
        {
            _TaskContext::task_onPreExecute();
            func_();
            _TaskContext::task_onPostExecute();

            task_base::setDone();
            notifier_type::notify();
        }

        //------------------------------------------------------------------------------------------
        virtual void executeSingleThreaded() override final
        {
            if (task_base::tryGrab())
            {
                _TaskContext::task_onPreExecute();
                func_();
                _TaskContext::task_onPostExecute();

                done_.store(true);
                notifier_type::notify();
            }
        }

        //------------------------------------------------------------------------------------------
        virtual void wait() override final
        {
            notifier_type::wait();
        }

        //------------------------------------------------------------------------------------------
        virtual void activeWait() override final
        {
            if (task_base::tryGrab())
            {
                execute();
            }
            else
            {
                wait();
            }
        }

    private:
        _Func func_;
    };
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Type     : user defined
    // Context  : user defined
    template<task_type _TaskType, typename _TaskContext, typename _Func>
    inline auto make_task(const std::string &name, _Func &&func, task_priority priority)
    {
        return std::make_shared<task<_TaskType, _TaskContext, std::decay_t<_Func>>>(name, std::forward<_Func>(func), priority);
    }
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Type     : waitable
    // Context  : user defined
    template<typename _TaskContext, typename _Func>
    inline auto make_task(const std::string &name, _Func &&f, task_priority priority)
    {
        return make_task<task_type::waitable, _TaskContext, _Func>(name, std::forward<_Func>(f), priority);
    }

    //----------------------------------------------------------------------------------------------
    // Type     : fire_and_forget
    // Context  : user defined
    template<typename _TaskContext, typename _Func>
    inline auto make_task_item(const std::string &name, _Func &&f, task_priority priority = task_priority::inherit)
    {
        return make_task<task_type::fire_and_forget, _TaskContext, _Func>(name, std::forward<_Func>(f), priority);
    }

} /*oqpi*/
