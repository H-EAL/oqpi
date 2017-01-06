#include <iostream>
#include <queue>
#include <mutex>

#include "oqpi.hpp"

#include "cqueue.hpp"
#include "timer_contexts.hpp"

using thread = oqpi::thread_interface;
using semaphore = oqpi::semaphore_interface;

template<typename T>
using cqueue = qqueue<T, std::mutex>;

using namespace std::chrono_literals;

using scheduler_type = oqpi::scheduler<cqueue>;

using tc = oqpi::task_context_container<timer_task_context>;
using gc = oqpi::group_context_container<timer_group_context>;

using oqpi_tk = oqpi::helpers<scheduler_type, gc, tc>;


static std::mutex coutMutex;
void SleepFor(int)
{
    thread_local volatile int a = 1000;
    int b = a;
    while (b--);
}

int main()
{
    const int STACK_SIZE = 0;

    const oqpi::worker_config workersConfig[] =
    {
    { oqpi::thread_attributes("WT(0) - ", STACK_SIZE, oqpi::core_affinity::core0, oqpi::thread_priority::highest), oqpi::worker_priority::wprio_any, 1},
    { oqpi::thread_attributes("WT(1) - ", STACK_SIZE, oqpi::core_affinity::core1, oqpi::thread_priority::highest), oqpi::worker_priority::wprio_any, 1},
    { oqpi::thread_attributes("WT(2) - ", STACK_SIZE, oqpi::core_affinity::core2, oqpi::thread_priority::highest), oqpi::worker_priority::wprio_any, 1},
    { oqpi::thread_attributes("WT(3) - ", STACK_SIZE, oqpi::core_affinity::core3, oqpi::thread_priority::highest), oqpi::worker_priority::wprio_any, 1},
    };
    
    oqpi_tk::scheduler_.registerWorkers<thread, semaphore>(workersConfig);
    oqpi_tk::scheduler_.start();


    auto t1 = oqpi::make_task<tc>("MyWaitableTask", oqpi::task_priority::high, [](int a, int b)
    {
        int somme = a + b;
        return somme;
    }, 20, 35);

    oqpi_tk::schedule_task(t1);

    auto t2 = oqpi::make_task<tc>("MyFireAndForgetTask", oqpi::task_priority::high,
        [](int somme)
        {
            return std::string("haha somme = " + std::to_string(somme));
        }
        , t1->waitForResult()
    );

    oqpi_tk::schedule_task(t2);
    t2->waitForResult();

    auto tg = oqpi_tk::make_parallel_group<oqpi::task_type::waitable>("MyFork", oqpi::task_priority::normal, 5);
    int32_t t = 0;
    tg->addTask(oqpi_tk::make_task_item("MyFT1", oqpi::task_priority::normal, SleepFor, t += 10));
    tg->addTask(oqpi_tk::make_task_item("MyFT2", oqpi::task_priority::normal, SleepFor, t += 10));
    tg->addTask(oqpi_tk::make_task_item("MyFT3", oqpi::task_priority::normal, SleepFor, t += 10));
    tg->addTask(oqpi_tk::make_task_item("MyFT4", oqpi::task_priority::normal, SleepFor, t += 10));

    oqpi_tk::schedule_task(oqpi::task_handle(tg));


    //const auto part = oqpi::simple_partitioner(40, 4);
    //oqpi::parallel_for<gc, tc>(oqpi_tk::scheduler_, "ParallelFor", part, oqpi::task_priority::normal, SleepFor);

    tg->wait();
    oqpi::this_thread::sleep_for(50ms);

    oqpi_tk::scheduler_.stop();

    return 0;
}