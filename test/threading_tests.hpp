
//--------------------------------------------------------------------------------------------------
TEST_CASE("Threading.", "[threading]")
{
    // oqpi::thread() will create a non-joinable thread.
    // If we try to detach or join it, it should simply issue a warning. 
    CHECK(oqpi::thread().joinable() == false);

    CHECK_NOTHROW(oqpi::thread().detach());

    CHECK_NOTHROW(oqpi::thread().join());

    auto coreAffinityMask       = oqpi::core_affinity(oqpi::core0 | oqpi::core1);
    // The maximal stack size of the thread, 0 will use the system's default value
    constexpr auto stackSize    = 0u;
    auto priority               = oqpi::thread_priority::lowest;
    auto ta = oqpi::thread_attributes("Thread", stackSize, coreAffinityMask, priority);
    
     auto t = oqpi::thread(ta, []()
    {
        fibonacci(gValue);
    });

     REQUIRE(t.getCoreAffinityMask() == coreAffinityMask);
     REQUIRE(t.getPriority() == priority);

     coreAffinityMask = oqpi::core_affinity(oqpi::core3);
     t.setCoreAffinityMask(coreAffinityMask);
     REQUIRE(t.getCoreAffinityMask() == coreAffinityMask);

     t.setCoreAffinity(4);
     REQUIRE(t.getCoreAffinityMask() == oqpi::core_affinity(oqpi::core4));

     priority = oqpi::thread_priority::above_normal;
     t.setPriority(priority);
     REQUIRE(t.getPriority() == priority);

     REQUIRE(t.joinable() == true);
     t.join();
}
