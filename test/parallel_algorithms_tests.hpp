
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
void test_parallel_algorithms()
{
    test_parallel_for();

    test_parallel_for_task();

    test_parallel_for_each();

    test_partitioners();
}

//--------------------------------------------------------------------------------------------------
TEST_CASE("Parallel algorithms (parallel for, partitioners).", "[scheduling]")
{
    CHECK_NOTHROW(test_parallel_algorithms());
}
