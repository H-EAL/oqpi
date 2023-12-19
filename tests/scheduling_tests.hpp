
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

    auto spTask = oqpi_tk::make_task("FibonacciPerCore", oqpi::task_priority::normal, []
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
void test_scheduling()
{
    test_unit_task_result();

    test_unit_task();

    test_multiple_unit_tasks();

    test_sequence_group();

    test_parallel_group();

    test_sequence_of_parallel_groups();
}

//--------------------------------------------------------------------------------------------------
TEST_CASE("Scheduling (tasks, sequence groups, parallel groups).", "[scheduling]")
{
    CHECK_NOTHROW(test_scheduling());
}
