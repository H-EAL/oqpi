#pragma once

#include "oqpi/scheduling.hpp"

namespace oqpi {

    namespace details {

        //------------------------------------------------------------------------------------------
        template<typename _Function>
        struct needs_batch_index
        {
            struct yes { char a;    };
            struct no  { char a[2]; };

            template <typename T>
            static yes test(decltype(std::declval<T>()(int(0), int(0)))*);

            template <typename>
            static no test(...);

            static const bool value = sizeof(test<_Function>(nullptr)) == sizeof(yes);
        };
        //------------------------------------------------------------------------------------------
        template<typename _Function, bool _WithBatchIndex = needs_batch_index<_Function>::value>
        struct parallel_for_caller;
        //------------------------------------------------------------------------------------------
        template<typename _Function>
        struct parallel_for_caller<_Function, false>
        {
            static void do_call(_Function &&func, int, int elementIndex)
            {
                func(elementIndex);
            }
        };
        //------------------------------------------------------------------------------------------
        template<typename _Function>
        struct parallel_for_caller<_Function, true>
        {
            static void do_call(_Function &&func, int batchIndex, int elementIndex)
            {
                func(batchIndex, elementIndex);
            }
        };
        //------------------------------------------------------------------------------------------
        template<typename _Function>
        inline void parallel_for_call(_Function &&func, int batchIndex, int elementIndex)
        {
            parallel_for_caller<_Function>::do_call(std::forward<_Function>(func), batchIndex, elementIndex);
        }
        //------------------------------------------------------------------------------------------

    } /*details*/


    //----------------------------------------------------------------------------------------------
    template<task_type _TaskType, typename _GroupContext, typename _TaskContext, typename _Scheduler, typename _Function, typename _Partitioner>
    inline std::shared_ptr<parallel_group<_Scheduler, _TaskType, _GroupContext>>
        make_parallel_for_task_group(_Scheduler &sc, const std::string &name, const _Partitioner &partitioner, task_priority prio, _Function &&func)
    {
        if (!partitioner.isValid())
            return nullptr;

        const auto nbElements = partitioner.elementCount();
        const auto nbBatches  = partitioner.batchCount();
        const auto &groupName = name + " (" + std::to_string(nbElements) + " items)";
        auto spTaskGroup      = make_parallel_group<_TaskType, _GroupContext>(sc, groupName, prio, nbBatches);
        auto spPartitioner    = std::make_shared<_Partitioner>(partitioner);

        for (auto batchIndex = 0; batchIndex < nbBatches; ++batchIndex)
        {
            const auto &taskName = "Batch " + std::to_string(batchIndex + 1) + "/" + std::to_string(nbBatches);
            auto taskHandle = make_task_item<_TaskContext>(taskName, prio,
                [batchIndex, func, spPartitioner]()
            {
                int32_t first = 0;
                int32_t last  = 0;
                while (spPartitioner->getNextValidRange(first, last))
                {
                    for (auto elementIndex = first; elementIndex != last; ++elementIndex)
                    {
                        details::parallel_for_call(func, batchIndex, elementIndex);
                    }
                }
            });

            spTaskGroup->addTask(std::move(taskHandle));
        } 

        return spTaskGroup;
    }
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    template<typename _GroupContext, typename _TaskContext, typename _Scheduler, typename _Function, typename _Partitioner>
    inline void parallel_for(_Scheduler &sc, const std::string &name, const _Partitioner &partitioner, task_priority prio, _Function &&func)
    {
        auto spTaskGroup = make_parallel_for_task_group<task_type::waitable, _GroupContext, _TaskContext>(sc, name, partitioner, prio, std::forward<_Function>(func));
        if(spTaskGroup)
        {
            sc.add(task_handle(spTaskGroup)).activeWait();
        }
    }
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
