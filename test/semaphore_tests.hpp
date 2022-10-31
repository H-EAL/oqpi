
TEST_CASE("Semaphores.", "[semaphore]")
{
    SECTION("Local Semaphore.")
    {
        const auto initCount  = 2u;
        const auto maxCount   = 3u;

        auto semaphore = oqpi::semaphore("", oqpi::sync_object_creation_options::open_or_create, initCount, maxCount);
        REQUIRE(semaphore.isValid());

        auto success = semaphore.tryWait(); // semaphoreCount = 1.
        REQUIRE(success);

        success = semaphore.tryWait(); // semaphoreCount = 0.
        REQUIRE(success);

        success = semaphore.tryWait(); // semaphoreCount = 0.
        REQUIRE(!success);

        semaphore.notifyOne(); // Semaphore count = 1.

        success = semaphore.tryWait(); // semaphoreCount = 0.
        REQUIRE(success);

        semaphore.notifyAll(); // semaphoreCount = 3.

        success = semaphore.tryWait() // semaphoreCount = 2.
            && semaphore.tryWait() // semaphoreCount = 1.
            && semaphore.tryWait(); // semaphoreCount = 0.

        REQUIRE(success);
    }
    SECTION("Global Semaphore.")
    {
        const auto initCount  = 2u;
        const auto maxCount   = 3u;

        auto semaphore = oqpi::global_semaphore("/Global\\oqpiTestSemaphore", oqpi::sync_object_creation_options::open_existing);
        REQUIRE(!semaphore.isValid());

        semaphore = oqpi::global_semaphore("/Global\\oqpiTestSemaphore", oqpi::sync_object_creation_options::create_if_nonexistent, initCount, maxCount);
        REQUIRE(semaphore.isValid());

        auto semaphore2 = oqpi::global_semaphore("/Global\\oqpiTestSemaphore", oqpi::sync_object_creation_options::open_existing);
        REQUIRE(semaphore2.isValid());

        REQUIRE(semaphore.getName() == semaphore2.getName());

        auto success = semaphore.tryWait(); // semaphoreCount = 1.
        REQUIRE(success);

        success = semaphore2.tryWait(); // semaphoreCount = 0.
        REQUIRE(success);

        success = semaphore2.tryWait(); // semaphoreCount = 0.
        REQUIRE(!success);

        semaphore2.notifyOne(); // Semaphore count = 1.

        success = semaphore.tryWait(); // semaphoreCount = 0.
        REQUIRE(success);

        semaphore.notifyAll(); // semaphoreCount = 3.

        success = semaphore2.tryWait() // semaphoreCount = 2.
            && semaphore2.tryWait() // semaphoreCount = 1.
            && semaphore2.tryWait(); // semaphoreCount = 0.

        REQUIRE(success);
    }
}
