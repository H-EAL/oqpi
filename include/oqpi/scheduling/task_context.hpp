#pragma once

#include "oqpi/scheduling/context_container.hpp"
#include "oqpi/scheduling/task_base.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    class task_base;
    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    class task_context_base
    {
    public:
        task_context_base(task_base *pOwner)
            : pOwner_(pOwner)
        {}

    public:
        inline task_base* owner() const { return pOwner_; }

    public:
        inline void onPreExecute() {}
        inline void onPostExecute() {}

    private:
        task_base *pOwner_;
    };
    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    template<typename... _ContextList>
    using task_context_container = context_container<task_base, _ContextList...>;
    //----------------------------------------------------------------------------------------------

} /*oqpi*/