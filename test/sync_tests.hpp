
TEST_CASE("Sync.", "[sync]")
{
    SECTION("wait_indefinitely_for_any")
    {
        // Make two events and two semaphores. Only the local semaphore is in a signaled state.
        // wait_indefinitely_for_any should return the index number of that semaphore i.e. 2.

        const auto autoResetEvent
            = oqpi::auto_reset_event("Local\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);

        const auto globalManualResetEvent
            = oqpi::global_manual_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);

        auto initCount = 1;

        const auto semaphore
            = oqpi::semaphore("Local\\oqpiTestSemaphore", oqpi::sync_object_creation_options::create_if_nonexistent, initCount);

        initCount = 0;

        const auto globalSemaphore
            = oqpi::global_semaphore("Global\\oqpiTestSemaphore", oqpi::sync_object_creation_options::create_if_nonexistent, initCount);

        const auto result = oqpi::sync::wait_indefinitely_for_any(autoResetEvent, globalManualResetEvent, semaphore, globalSemaphore);
        
        REQUIRE(result == 2);
    }
}
