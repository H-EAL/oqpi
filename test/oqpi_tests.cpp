 #include <iostream>
 
 #include "oqpi.hpp"
 
using thread = oqpi::thread_interface;

int main()
{
    const auto attr = oqpi::thread_attributes{ "MyThread", (std::numeric_limits<uint32_t>::min)(), oqpi::core_affinity::all_cores, oqpi::thread_priority::highest };
    thread th{ attr, [](const char *c, int i, float f)
    {
        std::cout << c << std::endl;
        std::cout << i << std::endl;
        std::cout << f << std::endl;

        volatile bool a = true;
        while (a)
        {
            using namespace std::chrono_literals;
            std::cout << oqpi::this_thread::get_current_core() << std::endl;
            oqpi::this_thread::sleep_for(500ms);
            static volatile int b = 0;
            b++;
        }


    }, "haha", 43, 8.f };    
    
    //const auto t = thread::hardware_concurrency();
    //th.setCoreAffinity(3);
    //const auto a = th.getCoreAffinityMask();
    //oqpi_checkf(a == oqpi::core3, "Core Affinity = 0x%x, expected 0x%x", int(a), int(oqpi::core3));


    auto t1 = oqpi::make_task<oqpi::default_task_context>("MyTask", [] {}, oqpi::task_priority::highest);
    auto t2 = oqpi::make_task_item<oqpi::default_task_context>("MyTask", [] {}, oqpi::task_priority::highest);

    t1->executeSingleThreaded();
    t2->executeSingleThreaded();


    th.join();

    return 0;
}