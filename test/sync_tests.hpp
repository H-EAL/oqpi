
TEST_CASE("Sync.", "[sync]")
{
    SECTION("wait_indefinitely_for_any")
    {
        // Make events, a mutex and semaphores. Only the local semaphore is in a signaled state.
        // wait_indefinitely_for_any should return the index number of that semaphore i.e. 3.

        auto lockOnCreation = true;

        auto mutex
            = oqpi::global_mutex("Global\\oqpiTestMutex", oqpi::sync_object_creation_options::create_if_nonexistent, lockOnCreation);

        auto autoResetEvent
            = oqpi::auto_reset_event("Local\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);

        auto globalManualResetEvent
            = oqpi::global_manual_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);

        auto initCount = 1;

        auto semaphore
            = oqpi::semaphore("Local\\oqpiTestSemaphore", oqpi::sync_object_creation_options::create_if_nonexistent, initCount);

        initCount = 0;

        auto globalSemaphore
            = oqpi::global_semaphore("Global\\oqpiTestSemaphore", oqpi::sync_object_creation_options::create_if_nonexistent, initCount);

        auto result = oqpi::sync::wait_indefinitely_for_any(mutex, autoResetEvent, globalManualResetEvent, semaphore, globalSemaphore);

        REQUIRE(result  == 3);
    }
}
