
TEST_CASE("Events.", "[event]")
{
    SECTION("Local events.")
    {
        SECTION("Auto Reset.")
        {
            auto autoResetEvent 
                = oqpi::auto_reset_event("", oqpi::sync_object_creation_options::create_if_nonexistent);
            REQUIRE(autoResetEvent.isValid());

            auto notifiedOnThread1 = false;
            auto notifiedOnThread2 = false;

            auto thread1 = oqpi::thread("autoResetThread1", [&autoResetEvent, &notifiedOnThread1]()
            {
                notifiedOnThread1 = autoResetEvent.waitFor(std::chrono::seconds(1));
            });

            auto thread2 = oqpi::thread("autoResetThread2", [&autoResetEvent, &notifiedOnThread2]()
            {
                notifiedOnThread2 = autoResetEvent.waitFor(std::chrono::seconds(1));
            });

            autoResetEvent.notify();

            thread1.join();
            thread2.join();

            // Either thread1 or thread2 got notified.
            const auto success = (notifiedOnThread1 && !notifiedOnThread2) || (!notifiedOnThread1 && notifiedOnThread2);
            REQUIRE(success);

            // As this is an auto reset event it should be automatically set to the non_signalled state after being signaled and waited upon once.
            auto notified = autoResetEvent.waitFor(std::chrono::microseconds(1));
            REQUIRE(!notified);
        }
        SECTION("Manual Reset.")
        {
            auto manualResetEvent 
                = oqpi::manual_reset_event("", oqpi::sync_object_creation_options::create_if_nonexistent);
            REQUIRE(manualResetEvent.isValid());

            auto notifiedOnThread1 = false;
            auto notifiedOnThread2 = false;

            auto thread1 = oqpi::thread("manualResetThread1", [&manualResetEvent, &notifiedOnThread1]()
            {
                notifiedOnThread1 = manualResetEvent.waitFor(std::chrono::seconds(1));
            });

            auto thread2 = oqpi::thread("manualResetThread2", [&manualResetEvent, &notifiedOnThread2]()
            {
                notifiedOnThread2 = manualResetEvent.waitFor(std::chrono::seconds(1));
            });

            manualResetEvent.notify();

            thread1.join();
            thread2.join();

            // Both thread1 or thread2 should have gotten notified.
            const auto success = notifiedOnThread1 && notifiedOnThread2;
            REQUIRE(success);

            // As this is a manual reset event, it should only be set to the nonsignaled state after an explicit call to reset(). 
            // Therefore in the call below it should be notified.
            auto notified = manualResetEvent.waitFor(std::chrono::microseconds(1));
            REQUIRE(notified);

            manualResetEvent.reset();
            notified = manualResetEvent.waitFor(std::chrono::microseconds(1));
            REQUIRE(!notified);
        }
    }

    SECTION("Global events.")
    {
        SECTION("Auto Reset.")
        {
            auto autoResetEvent
                = oqpi::global_auto_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);
            REQUIRE(autoResetEvent.isValid());

            auto autoResetEvent2
                = oqpi::global_auto_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);
            REQUIRE(!autoResetEvent2.isValid());

            autoResetEvent2
                = oqpi::global_auto_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::open_existing);
            REQUIRE(autoResetEvent2.isValid());

            autoResetEvent.notify();
            auto notified = autoResetEvent2.waitFor(std::chrono::seconds(10));

            REQUIRE(notified);

            notified = autoResetEvent.waitFor(std::chrono::microseconds(1));
            REQUIRE(!notified);
        }
        SECTION("Manual Reset.")
        {
            auto manualResetEvent
                = oqpi::global_manual_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);
            REQUIRE(manualResetEvent.isValid());

            auto manualResetEvent2
                = oqpi::global_manual_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::create_if_nonexistent);
            REQUIRE(!manualResetEvent2.isValid());

            manualResetEvent2
                = oqpi::global_manual_reset_event("Global\\oqpiTestEvent", oqpi::sync_object_creation_options::open_existing);
            REQUIRE(manualResetEvent2.isValid());

            manualResetEvent.notify();
            auto notified = manualResetEvent2.waitFor(std::chrono::seconds(10));

            REQUIRE(notified);

            notified = manualResetEvent.waitFor(std::chrono::microseconds(1));
            REQUIRE(notified);

            manualResetEvent2.reset();
            notified = manualResetEvent.waitFor(std::chrono::microseconds(1));

            REQUIRE(!notified);
        }
    }
}