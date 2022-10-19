
//--------------------------------------------------------------------------------------------------
TEST_CASE("Threading.", "[threading]")
{
    // oqpi::thread() will create a non-joinable thread.
    // If we try to detach or join it, it should simply issue a warning. 
    CHECK(oqpi::thread().joinable() == false);

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

     coreAffinityMask = oqpi::core_affinity(oqpi::core3);
     t.setCoreAffinityMask(coreAffinityMask);
     CHECK(t.getCoreAffinityMask() == coreAffinityMask);

     t.setCoreAffinity(4);
     CHECK(t.getCoreAffinityMask() == oqpi::core_affinity(oqpi::core4));

     priority = oqpi::thread_priority::above_normal;
     t.setPriority(priority);
     CHECK(t.getPriority() == priority);

     REQUIRE(t.joinable() == true);
     t.join();
}
