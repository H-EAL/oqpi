#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"


#define OQPI_USE_DEFAULT
#include "oqpi.hpp"

#include "test_utils.hpp"


using namespace std::chrono_literals;

#define PROFILE_TASKS (1)
//--------------------------------------------------------------------------------------------------
// Define the toolkit we want to use
#if PROFILE_TASKS
    using gc            = oqpi::group_context_container<timer_group_context>;
    using tc            = oqpi::task_context_container<timer_task_context>;
    using oqpi_tk       = oqpi::helpers<oqpi::scheduler<concurrent_queue>, gc, tc>;
#else
    using oqpi_tk       = oqpi::default_helpers;
#endif
//--------------------------------------------------------------------------------------------------


TEST_CASE("Setup.", "[cleanup]")
{
    oqpi_tk::start_default_scheduler();
}

#include "threading_tests.hpp"

#include "scheduling_tests.hpp"

#include "parallel_algorithms_tests.hpp"

#include "synchronization_tests.hpp"


TEST_CASE("Cleanup.", "[cleanup]")
{
    oqpi_tk::stop_scheduler();
}
