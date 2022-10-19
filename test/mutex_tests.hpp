
TEST_CASE("Mutexes.", "[mutex]")
{
    SECTION("Global Mutex.")
    {
        auto mutex = oqpi::global_mutex("Global\\oqpiTestMutex", oqpi::sync_object_creation_options::open_existing);
        REQUIRE(!mutex.isValid());

        mutex = oqpi::global_mutex("Global\\oqpiTestMutex", oqpi::sync_object_creation_options::create_if_nonexistent);
        REQUIRE(mutex.isValid());

        auto mutex2 = oqpi::global_mutex("Global\\oqpiTestMutex", oqpi::sync_object_creation_options::open_existing);
        REQUIRE(mutex2.isValid());

        mutex.unlock();
        auto lockSucceeded = mutex2.tryLockFor(std::chrono::microseconds(1u));
        REQUIRE(lockSucceeded);

        mutex2.unlock();
        lockSucceeded = mutex.tryLockFor(std::chrono::microseconds(1u));
        REQUIRE(lockSucceeded);
    }
}
