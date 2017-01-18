#include <iostream>

#define OQPI_USE_DEFAULT
#include "oqpi.hpp"

#include "timer_contexts.hpp"


using namespace std::chrono_literals;

#define PROFILE_TASKS (1)
//--------------------------------------------------------------------------------------------------
// Define the toolkit we want to use
#if PROFILE_TASKS
    using gc            = oqpi::group_context_container<timer_group_context>;
    using tc            = oqpi::task_context_container<timer_task_context>;
    using oqpi_tk       = oqpi::helpers<oqpi::scheduler<concurrent_queue>, gc, tc>;
#else
    using oqpi_tk       = oqpi::default_helpers;
#endif
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
static constexpr auto gValue = 10000000ull;
static const auto gTaskCount = int(oqpi::thread::hardware_concurrency());
//--------------------------------------------------------------------------------------------------


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


//--------------------------------------------------------------------------------------------------
struct test
{
    test(const char *functionName)
    {
        std::cout << "-------------------------------------------------------------------" << std::endl;
        std::cout << functionName << std::endl;
        std::cout << "-------------------------------------------------------------------" << std::endl;
    }

    ~test()
    {
        timing_registry::get().dump();
        timing_registry::get().reset();
        std::cout << "-------------------------------------------------------------------" << std::endl;
        std::cout << std::endl << std::endl;
    }
};
//--------------------------------------------------------------------------------------------------
#define TEST_FUNC test __t(__FUNCTION__)
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
void test_unit_task_result()
{
    TEST_FUNC;

    auto spTask = oqpi_tk::make_task("FibonacciReturnResult", fibonacci, gValue);
    oqpi_tk::schedule_task(spTask);
    std::cout << "Fibonacci(" << gValue << ") = " << spTask->waitForResult() << std::endl;
}

//--------------------------------------------------------------------------------------------------
void test_unit_task()
{
    TEST_FUNC;

    auto spTask = oqpi_tk::make_task("Fibonacci4x", oqpi::task_priority::normal, []
    {
        for (int i = 0; i < gTaskCount; ++i)
        {
            volatile auto a = 0ull;
            a += fibonacci(gValue+a);
        }
    });
    oqpi_tk::schedule_task(spTask).wait();
}

//--------------------------------------------------------------------------------------------------
void test_multiple_unit_tasks()
{
    TEST_FUNC;

    std::vector<oqpi::task_handle> handles(gTaskCount);
    for (auto i = 0; i < gTaskCount; ++i)
    {
        handles[i] = oqpi_tk::schedule_task("Fibonacci_" + std::to_string(i), fibonacci, gValue);
    }
    for (auto &h : handles)
    {
        h.wait();
    }
}

//--------------------------------------------------------------------------------------------------
void test_sequence_group()
{
    TEST_FUNC;

    auto spSeq = oqpi_tk::make_sequence_group<oqpi::task_type::waitable>("Sequence");
    for (auto i = 0; i < gTaskCount; ++i)
    {
        auto spTask = oqpi_tk::make_task_item("FibonacciSeq" + std::to_string(i), fibonacci, gValue);
        spSeq->addTask(spTask);
    }
    oqpi_tk::schedule_task(oqpi::task_handle(spSeq)).wait();
}

//--------------------------------------------------------------------------------------------------
void test_parallel_group()
{
    TEST_FUNC;

    auto spFork = oqpi_tk::make_parallel_group<oqpi::task_type::waitable>("Fork", oqpi::task_priority::normal, gTaskCount);
    for (auto i = 0; i < gTaskCount; ++i)
    {
        auto spTask = oqpi_tk::make_task_item("FibonacciFork" + std::to_string(i), fibonacci, gValue);
        spFork->addTask(spTask);
    }
    oqpi_tk::schedule_task(oqpi::task_handle(spFork)).wait();
}

//--------------------------------------------------------------------------------------------------
void test_sequence_of_parallel_groups()
{
    TEST_FUNC;

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
}

//--------------------------------------------------------------------------------------------------
void test_parallel_for_task()
{
    TEST_FUNC;

    const auto prio = oqpi::task_priority::normal;
    const auto partitioner = oqpi::simple_partitioner(gTaskCount, oqpi_tk::scheduler().workersCount(prio));
    auto spParallelForGroup = oqpi_tk::make_parallel_for_task_group<oqpi::task_type::waitable>("FibonacciParallelForGroup", partitioner, prio,
        [](int32_t i)
    {
        volatile auto a = 0ull;
        a += fibonacci(gValue + a);
        a += i;
    });
    oqpi_tk::schedule_task(oqpi::task_handle(spParallelForGroup)).wait();
}

//--------------------------------------------------------------------------------------------------
void test_parallel_for()
{
    TEST_FUNC;

    oqpi_tk::parallel_for("FibonacciParallelFor", gTaskCount, [](int32_t i)
    {
        volatile auto a = 0ull;
        a += fibonacci(gValue + a);
        a += i;
    });
}

//--------------------------------------------------------------------------------------------------
void test_parallel_for_each()
{
    TEST_FUNC;

    std::vector<std::string> vec
    {
        "Lorem ipsum dolor",
        " sit amet, consectetur",
        " adipiscing elit.",
        "Nullam nulla sapien,",
        " mattis a egestas id,",
        " lobortis ut mauris.",
        "Proin consectetur finibus diam,",
        " eu maximus velit interdum sed.",
        "Quisque massa nibh, molestie vitae",
        " volutpat sed, lacinia non odio.",
        "Aenean dui enim, porttitor quis velit quis,",
        " lacinia viverra ex.",
        "Vivamus et libero sit amet orci consequat consectetur.",
        "Duis eu porttitor nunc,",
        " nec scelerisque mi.Cras efficitur lobortis elit, id",
        " scelerisque diam interdum ut.",
        "Quisque blandit sodales venenatis.",
        "Ut at purus congue dolor cursus posuere.",
        "Aliquam ac volutpat turpis, ac placerat nisl."
    };
    const auto vecSize = int32_t(vec.size());
    std::atomic<uint64_t> count = 0;

    oqpi_tk::parallel_for_each("FibonacciParallelForEach", vec,
        [&count](const std::string &s)
    {
        uint64_t total = 0;
        for (auto c : s)
        {
            total += uint64_t(c) * 100;
        }
        count += fibonacci(total);
    });
}

//--------------------------------------------------------------------------------------------------
void test_partitioners()
{
    TEST_FUNC;

    std::vector<uint64_t> vec
    {
        56,96,63,25,84,20,54,86,45,89,32,44,58,99,85,32,
        56985635,58975563,96583265,85452542,63254125,78965458,52365425,85212547,12568542,33556396,66996655,88888888,89658756,56325426,66999988,51122554,
        236,862,966,666,888,544,887,211,544,877,555,445,845,215,548,655,
        2366,4862,8966,5666,3888,1454,8287,2211,5644,8787,5955,4845,8845,2145,5248,6555
    };
    const auto vecSize = int32_t(vec.size());
    std::vector<uint64_t> fib(vecSize);

    auto h = oqpi_tk::schedule_task("FibTransform", [&vec, &fib]
    {
        std::transform(vec.begin(), vec.end(), fib.begin(), [](uint64_t i) { return fibonacci(i); });
    });
    h.wait();

    const auto prio = oqpi::task_priority::normal;
    const auto simplePartitioner = oqpi::simple_partitioner(vecSize, oqpi_tk::scheduler().workersCount(prio));
    oqpi_tk::parallel_for_each("FibonacciParallelForEach", vec, simplePartitioner, prio,
        [](uint64_t &i)
    {
        volatile auto a = 0ull;
        a += fibonacci(i + a);
    });

    const auto atomicPartitioner = oqpi::atomic_partitioner(vecSize, 1, oqpi_tk::scheduler().workersCount(prio));
    oqpi_tk::parallel_for_each("FibonacciParallelForEach", vec, atomicPartitioner, prio,
        [](uint64_t &i)
    {
        volatile auto a = 0ull;
        a += fibonacci(i + a);
    });
}


//--------------------------------------------------------------------------------------------------
int main()
{
    oqpi_tk::start_default_scheduler();

    test_unit_task_result();

    test_unit_task();

    test_multiple_unit_tasks();

    test_sequence_group();

    test_parallel_group();

    test_sequence_of_parallel_groups();

    test_parallel_for();

    test_parallel_for_task();

    test_parallel_for_each();

    test_partitioners();

    oqpi_tk::stop_scheduler();
}
