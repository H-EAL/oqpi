#pragma once

#include <tuple>
#include "oqpi/scheduling/task_base.hpp"
#include "oqpi/scheduling/task_result.hpp"
#include "oqpi/scheduling/task_notifier.hpp"


namespace oqpi {

    template<task_type _TaskType, typename _TaskContext, typename _Tuple, typename _ReturnType>
    class task final
        : public task_base
        , public task_result<_ReturnType>
        , public notifier<_TaskType>
        , public _TaskContext
    {
        //------------------------------------------------------------------------------------------
        using self_type         = task<_TaskType, _TaskContext, _Tuple, _ReturnType>;
        using task_result_type  = task_result<_ReturnType>;
        using notifier_type     = notifier<_TaskType>;

    public:
        //------------------------------------------------------------------------------------------
        task(std::string name, task_priority priority, _Tuple tuple)
            : task_base(std::move(name), priority)
            , notifier_type(name_)
            , _TaskContext(this)
            , tuple_(std::move(tuple))
        {}

        //------------------------------------------------------------------------------------------
        // Movable
        task(self_type &&other)
            : task_base(std::move(other))
            , notifier_type(std::move(other))
            , _TaskContext(std::move(other))
            , tuple_(std::move(other.tuple_))
        {}

        //------------------------------------------------------------------------------------------
        self_type& operator =(self_type &&rhs)
        {
            if (this != &rhs)
            {
                task_base::operator =(std::move(rhs));
                notifier_type::operator =(std::move(rhs));
                _TaskContext::operator =(std::move(rhs));
                tuple_ = std::move(rhs.tuple_);
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
            invoke();
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
                invoke();
                _TaskContext::task_onPostExecute();

                done_.store(true);
                notifier_type::notify();
            }
        }

        //------------------------------------------------------------------------------------------
        virtual void wait() const override final
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

        //------------------------------------------------------------------------------------------
        _ReturnType getResult() const
        {
            oqpi_checkf(task_base::isDone(), "Trying to get the result of an unfinished task (%s).", task_base::getName().c_str());
            return task_result_type::getResult();
        }

        //------------------------------------------------------------------------------------------
        _ReturnType waitForResult() const
        {
            wait();
            return getResult();
        }

    private:
        //------------------------------------------------------------------------------------------
        inline void invoke()
        {
            task_result_type::run([this]
            {
                return invokeTuple(std::make_integer_sequence<size_t, std::tuple_size<typename _Tuple>::value>());
            });
        }
        //------------------------------------------------------------------------------------------
        template<size_t... _Indices>
        inline auto invokeTuple(std::integer_sequence<size_t, _Indices...>)
        {
            return std::invoke(std::move(std::get<_Indices>(tuple_))...);
        }

    private:
        _Tuple tuple_;
    };
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Type     : user defined
    // Context  : user defined
    template<task_type _TaskType, typename _TaskContext, typename _Func, typename... _Args>
    inline auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
    {
        using tuple_type    = std::tuple<std::decay_t<_Func>, std::decay_t<_Args>...>;
        using return_type   = typename std::result_of<_Func(_Args...)>::type;
        using task_type     = task<_TaskType, _TaskContext, tuple_type, return_type>;
        return std::make_shared<task_type>
        (
            name,
            priority,
            tuple_type(std::forward<_Func>(func), std::forward<_Args>(args)...)
        );
    }
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Type     : waitable
    // Context  : user defined
    template<typename _TaskContext, typename _Func, typename... _Args>
    inline auto make_task(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
    {
        return make_task<task_type::waitable, _TaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
    }

    //----------------------------------------------------------------------------------------------
    // Type     : fire_and_forget
    // Context  : user defined
    template<typename _TaskContext, typename _Func, typename... _Args>
    inline auto make_task_item(const std::string &name, task_priority priority, _Func &&func, _Args &&...args)
    {
        return make_task<task_type::fire_and_forget, _TaskContext>(name, priority, std::forward<_Func>(func), std::forward<_Args>(args)...);
    }

} /*oqpi*/
