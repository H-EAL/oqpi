#include <iostream>
#include <queue>
#include <mutex>

#include "oqpi.hpp"

#include "cqueue.hpp"
#include "timer_contexts.hpp"

using namespace std::chrono_literals;

//--------------------------------------------------------------------------------------------------
// Types
using thread            = oqpi::thread_interface;
using semaphore         = oqpi::semaphore_interface;
template<typename T>
using cqueue            = qqueue<T, std::mutex>;
using scheduler_type    = oqpi::scheduler<cqueue>;
using gc                = oqpi::group_context_container<timer_group_context>;
using tc                = oqpi::task_context_container<timer_task_context>;
using oqpi_tk           = oqpi::helpers<scheduler_type, gc, tc>;


//--------------------------------------------------------------------------------------------------
void setup_environment()
{
//     std::cout << "-------------------------------------------------------------------" << std::endl;
//     std::cout << __FUNCTION__ << std::endl;
//     std::cout << "-------------------------------------------------------------------" << std::endl;
    const auto workerCount = thread::hardware_concurrency();
    for (auto i = 0u; i < workerCount; ++i)
    {
        auto config = oqpi::worker_config{};
        config.threadAttributes.coreAffinityMask_   = oqpi::core_affinity(1 << i);
        config.threadAttributes.name_               = "oqpi::worker_" + std::to_string(i);
        config.threadAttributes.priority_           = oqpi::thread_priority::highest;
        config.workerPrio                           = oqpi::worker_priority::wprio_any;
        config.count                                = 1;
        oqpi_tk::scheduler().registerWorker<thread, semaphore>(config);
    }

    oqpi_tk::scheduler().start();
//    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
static constexpr auto gValue = 10000000ull;
static constexpr auto gTaskCount = 4;

//--------------------------------------------------------------------------------------------------
uint64_t fibonacci(uint64_t n)
{
    uint64_t a = 1, b = 1;
    for (uint64_t i = 3; i <= n; ++i)
    {
        uint64_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

//--------------------------------------------------------------------------------------------------
void test_unit_task_result()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    auto spTask = oqpi_tk::make_task("FibonacciReturnResult", oqpi::task_priority::normal, fibonacci, gValue);
    oqpi_tk::schedule_task(spTask);
    std::cout << "Fibonacci(" << gValue << ") = " << spTask->waitForResult() << std::endl;
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void test_unit_task()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    auto spTask = oqpi_tk::make_task("Fibonacci4x", oqpi::task_priority::normal, []
    {
        for (int i = 0; i < gTaskCount; ++i)
        {
            volatile auto a = 0ull;
            a += fibonacci(gValue+a);
        }
    });
    oqpi_tk::schedule_task(spTask).wait();
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void test_multiple_unit_tasks()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    oqpi::task_handle handles[gTaskCount];
    for (auto i = 0; i < gTaskCount; ++i)
    {
        handles[i] = oqpi_tk::make_task("Fibonacci4xWait", oqpi::task_priority::normal, fibonacci, gValue);
        oqpi_tk::schedule_task(handles[i]);
    }
    for (auto i = 0; i < gTaskCount; ++i)
    {
        handles[i].wait();
    }
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void test_sequence_group()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    auto spSeq = oqpi_tk::make_sequence_group<oqpi::task_type::waitable>("Sequence");
    for (auto i = 0; i < gTaskCount; ++i)
    {
        auto spTask = oqpi_tk::make_task_item("FibonacciSeq" + std::to_string(i), oqpi::task_priority::normal, fibonacci, gValue);
        spSeq->addTask(spTask);
    }
    oqpi_tk::schedule_task(oqpi::task_handle(spSeq)).wait();
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void test_parallel_group()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    auto spFork = oqpi_tk::make_parallel_group<oqpi::task_type::waitable>("Fork", oqpi::task_priority::normal, gTaskCount);
    for (auto i = 0; i < gTaskCount; ++i)
    {
        auto spTask = oqpi_tk::make_task_item("FibonacciFork" + std::to_string(i), oqpi::task_priority::normal, fibonacci, gValue);
        spFork->addTask(spTask);
    }
    oqpi_tk::schedule_task(oqpi::task_handle(spFork)).wait();
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void test_sequence_of_parallel_groups()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    auto spSeq = oqpi_tk::make_sequence_group<oqpi::task_type::waitable>("Sequence");
    for (auto i = 0; i < gTaskCount; ++i)
    {
        auto spFork = oqpi_tk::make_parallel_group<oqpi::task_type::fire_and_forget>("Fork" + std::to_string(i), oqpi::task_priority::normal, gTaskCount);
        for (auto j = 0; j < gTaskCount; ++j)
        {
            auto spTask = oqpi_tk::make_task_item("Fibonacci_" + std::to_string(i*10+j), oqpi::task_priority::normal, fibonacci, gValue);
            spFork->addTask(spTask);
        }
        spSeq->addTask(oqpi::task_handle(spFork));
    }
    oqpi_tk::schedule_task(oqpi::task_handle(spSeq)).wait();
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void test_parallel_for()
{
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    const auto prio = oqpi::task_priority::normal;
    const auto partitioner = oqpi::simple_partitioner(gTaskCount, oqpi_tk::scheduler().workersCount(prio));
    oqpi_tk::parallel_for("FibonacciParallelFor", partitioner, prio,
        [](int32_t)
    {
        volatile auto a = 0ull;
        a += fibonacci(gValue + a);
    });
    timing_registry::get().dump();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
void clean_up_environement()
{
    oqpi::this_thread::sleep_for(500ms);
    oqpi_tk::scheduler().stop();
}


//--------------------------------------------------------------------------------------------------
int main()
{
    setup_environment();
    oqpi::this_thread::sleep_for(5ms);
    test_unit_task_result();
    oqpi::this_thread::sleep_for(5ms);
    test_unit_task();
    oqpi::this_thread::sleep_for(5ms);
    test_multiple_unit_tasks();
    oqpi::this_thread::sleep_for(5ms);
    test_sequence_group();
    oqpi::this_thread::sleep_for(5ms);
    test_parallel_group();
    oqpi::this_thread::sleep_for(5ms);
    test_sequence_of_parallel_groups();
    oqpi::this_thread::sleep_for(5ms);
    test_parallel_for();
    oqpi::this_thread::sleep_for(5ms);
    clean_up_environement();
}