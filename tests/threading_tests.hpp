
//--------------------------------------------------------------------------------------------------
static oqpi::core_affinity coreNumberToAffinity(uint32_t coreNumber)
{
    return static_cast<oqpi::core_affinity>(pow(2, coreNumber));
}

//--------------------------------------------------------------------------------------------------
TEST_CASE("Threading.", "[threading]")
{
    // oqpi::thread() will create a non-joinable thread.
    // If we try to detach or join it, it should simply issue a warning. 
    CHECK(oqpi::thread().joinable() == false);

    const auto numLogicalCores = oqpi::thread::hardware_concurrency();
    // I assume that there are least two cores to test with below.
    REQUIRE(numLogicalCores >= 2);

    auto coreAffinityMask       = oqpi::core_affinity(oqpi::core0 | oqpi::core1);
    // The maximal stack size of the thread, 0 will use the system's default value
    constexpr auto stackSize    = 0u;
    auto priority               = oqpi::thread_priority::lowest;
    const auto ta               = oqpi::thread_attributes("Thread", stackSize, coreAffinityMask, priority);
    
     auto t = oqpi::thread(ta, []()
    {
        fibonacci(gValue);
    });

     CHECK(t.getCoreAffinityMask() == coreAffinityMask);
     CHECK(t.getPriority() == priority);

     auto coreNum = numLogicalCores - 1;
     coreAffinityMask = coreNumberToAffinity(coreNum);

     t.setCoreAffinityMask(coreAffinityMask);
     CHECK(t.getCoreAffinityMask() == coreAffinityMask);

     coreNum = numLogicalCores - 2;
     coreAffinityMask = coreNumberToAffinity(coreNum);

    t.setCoreAffinity(coreNum);
     CHECK(t.getCoreAffinityMask() == coreAffinityMask);

     priority = oqpi::thread_priority::above_normal;
     t.setPriority(priority);
     CHECK(t.getPriority() == priority);

     REQUIRE(t.joinable() == true);
     t.join();
}
